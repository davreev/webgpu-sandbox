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

    // Get the device's queue
    WGPUQueue const queue = wgpuDeviceGetQueue(gpu.device);

    struct CmdContext
    {
        enum Status
        {
            Status_Pending = 0,
            Status_Success,
            Status_Failure,
        };

        WGPUBuffer src_buf;
        WGPUBuffer dst_buf;
        Status status;

        ~CmdContext()
        {
            wgpuBufferRelease(src_buf);
            wgpuBufferRelease(dst_buf);
        }
    };

    CmdContext cmd{};
    cmd.src_buf = make_buffer(gpu.device, 16, WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc);
    cmd.dst_buf = make_buffer(gpu.device, 16, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);

    // Submit command to copy data from one buffer to another
    {
        // Create command encoder
        WGPUCommandEncoder const encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        auto const drop_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // Issue command(s)
        wgpuCommandEncoderCopyBufferToBuffer(encoder, cmd.src_buf, 0, cmd.dst_buf, 0, 16);
        // ...
        // ...
        // ...

        // Encode commands
        WGPUCommandBuffer const command = wgpuCommandEncoderFinish(encoder, nullptr);
        auto const drop_command = defer([=]() { wgpuCommandBufferRelease(command); });

        // Submit the encoded command
        wgpuQueueSubmit(queue, 1, &command);
    }

    // Read dst buffer data back to host asynchronously
    auto const map_cb = [](WGPUBufferMapAsyncStatus status, void* userdata) {
        auto cmd = static_cast<CmdContext*>(userdata);

        if (status != WGPUBufferMapAsyncStatus_Success)
        {
            fmt::print("Failed to map dst buffer ({})\n", to_string(status));
            cmd->status = CmdContext::Status_Failure;
        }
        else
        {
            fmt::print("Successfully mapped dst buffer\n");
            cmd->status = CmdContext::Status_Success;

            // Ensure dst buffer is unmapped on scope exit
            auto const unmap_dst = defer([=]() { wgpuBufferUnmap(cmd->dst_buf); });

            // Print out the mapped buffer's data
            {
                auto data = static_cast<std::uint8_t const*>(
                    wgpuBufferGetConstMappedRange(cmd->dst_buf, 0, 16));

                fmt::print("dst_buf = [{}", data[0]);
                for (int i = 1; i < 16; ++i)
                    fmt::print(", {}", data[i]);
                fmt::print("]\n", data[15]);
            }
        }
    };
    wgpuBufferMapAsync(cmd.dst_buf, WGPUMapMode_Read, 0, 16, map_cb, &cmd);

    // Wait until async work is done
#ifdef __EMSCRIPTEN__
    while (cmd.status == CmdContext::Status_Pending)
        emscripten_sleep(100);
#else
    wgpuDevicePoll(gpu.device, true, nullptr);
#endif

    return (cmd.status == CmdContext::Status_Success) ? 0 : 1;
}
