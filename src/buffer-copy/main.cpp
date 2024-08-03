#include <cassert>
#include <cstdint>

#include <fmt/core.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#include <webgpu/wgpu.h>
#endif

#include <webgpu/webgpu.h>

#include <wgpu_utils.hpp>

#include "wgpu_config.h"

using namespace wgpu;

namespace
{

struct GpuContext
{
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    bool is_valid;
};

GpuContext make_gpu_context()
{
    GpuContext ctx{};

    // Create WebGPU instance
    ctx.instance = wgpuCreateInstance(nullptr);
    if (!ctx.instance)
    {
        fmt::print("Failed to create WebGPU instance\n");
        return ctx;
    }

    // Create WGPU adapter
    ctx.adapter = request_adapter(ctx.instance);
    if (!ctx.adapter)
    {
        fmt::print("Failed to get WebGPU adapter\n");
        return ctx;
    }

    // Create WebGPU device
    ctx.device = request_device(ctx.adapter);
    if (!ctx.device)
    {
        fmt::print("Failed to get WebGPU device\n");
        return ctx;
    }

    // Set error callback on device
    wgpuDeviceSetUncapturedErrorCallback(
        ctx.device,
        [](WGPUErrorType type, char const* msg, void* /*userdata*/) {
            fmt::print(
                "WebGPU device error: {} ({})\nMessage: {}\n",
                to_string(type),
                static_cast<int>(type),
                msg);
        },
        nullptr);

    ctx.is_valid = true;
    return ctx;
}

void release_gpu_context(GpuContext& ctx)
{
    wgpuDeviceRelease(ctx.device);
    wgpuAdapterRelease(ctx.adapter);
    wgpuInstanceRelease(ctx.instance);
    ctx = {};
}

struct CommandContext
{
    enum Status
    {
        Status_Pending = 0,
        Status_Success,
        Status_Failure,
    };

    struct
    {
        WGPUBuffer src;
        WGPUBuffer dst;
        std::size_t size;
    } buffers{};
    Status status{};
};

CommandContext make_command_context(GpuContext const& gpu, std::size_t const buffer_size)
{
    CommandContext cmd{};
    cmd.buffers = {
        make_buffer(gpu.device, buffer_size, WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc),
        make_buffer(gpu.device, buffer_size, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead),
        buffer_size,
    };
    return cmd;
}

void release_command_context(CommandContext& ctx)
{
    wgpuBufferRelease(ctx.buffers.src);
    wgpuBufferRelease(ctx.buffers.dst);
    ctx = {};
}

} // namespace

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu;

    // Create WebGPU context
    GpuContext gpu = make_gpu_context();
    if (!gpu.is_valid)
    {
        fmt::print("Failed to initialize WebGPU context\n");
        return 1;
    }
    auto const drop_gpu = defer([&]() { release_gpu_context(gpu); });

    // Create command context
    CommandContext cmd = make_command_context(gpu, 16);
    auto const drop_cmd = defer([&]() { release_command_context(cmd); });

    // Submit command to copy data from one buffer to another
    {
        // Create command encoder
        WGPUCommandEncoder const encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto const drop_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // Issue command
        wgpuCommandEncoderCopyBufferToBuffer(
            encoder,
            cmd.buffers.src,
            0,
            cmd.buffers.dst,
            0,
            cmd.buffers.size);

        // Encode commands
        WGPUCommandBuffer const command = wgpuCommandEncoderFinish(encoder, nullptr);
        auto const drop_command = defer([=]() { wgpuCommandBufferRelease(command); });

        // Submit the encoded command
        WGPUQueue const queue = wgpuDeviceGetQueue(gpu.device);
        wgpuQueueSubmit(queue, 1, &command);
    }

    // Read dst buffer data back to host asynchronously
    auto const map_cb = [](WGPUBufferMapAsyncStatus status, void* userdata) {
        auto cmd = static_cast<CommandContext*>(userdata);

        if (status != WGPUBufferMapAsyncStatus_Success)
        {
            fmt::print("Failed to map dst buffer ({})\n", to_string(status));
            cmd->status = CommandContext::Status_Failure;
        }
        else
        {
            fmt::print("Successfully mapped dst buffer\n");
            cmd->status = CommandContext::Status_Success;

            // Ensure the mapped buffer is unmapped on scope exit
            auto const unmap_dst = defer([=]() { wgpuBufferUnmap(cmd->buffers.dst); });

            // Print out the mapped buffer's data
            {
                std::size_t const n = cmd->buffers.size;
                auto data = static_cast<std::uint8_t const*>(
                    wgpuBufferGetConstMappedRange(cmd->buffers.dst, 0, n));

                fmt::print("dst_buf = [{}", data[0]);

                for (std::size_t i = 1; i < n; ++i)
                    fmt::print(", {}", data[i]);

                fmt::print("]\n", data[n - 1]);
            }
        }

#ifdef __EMSCRIPTEN__
        raise_event("resultReady");
#endif
    };
    wgpuBufferMapAsync(cmd.buffers.dst, WGPUMapMode_Read, 0, cmd.buffers.size, map_cb, &cmd);

    // Wait until async work is done
#ifdef __EMSCRIPTEN__
    wait_for_event("resultReady");
#else
    wgpuDevicePoll(gpu.device, true, nullptr);
#endif

    return (cmd.status == CommandContext::Status_Success) ? 0 : 1;
}
