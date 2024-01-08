#include <cassert>
#include <cstdint>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include <wgpu_utils.hpp>

#include "wgpu_config.h"

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu;

    // Create WebGPU instance
    WGPUInstance const instance = wgpuCreateInstance(nullptr);
    if (!instance)
    {
        fmt::print("Failed to create WebGPU instance\n");
        return 1;
    }
    auto const drop_instance = defer([=]() { wgpuInstanceRelease(instance); });

    // Get WebGPU adapter via the instance
    WGPUAdapter const adapter = request_adapter(instance);
    if (!adapter)
    {
        fmt::print("Failed to get WebGPU adapter\n");
        return 1;
    }
    auto const drop_adapter = defer([=]() { wgpuAdapterRelease(adapter); });

    // Get WebGPU device via the adapter
    WGPUDevice const device = request_device(adapter);
    if (!device)
    {
        fmt::print("Failed to get WebGPU device\n");
        return 1;
    }
    auto const drop_device = defer([=]() { wgpuDeviceRelease(device); });

    // Set error callback on device
    constexpr auto device_error_cb = [](WGPUErrorType type, char const* message, void* /*userdata*/)
    {
        fmt::print(
            "Device error: {} ({})\nMessage: {}\n",
            to_string(type),
            static_cast<int>(type),
            message);
    };
    wgpuDeviceSetUncapturedErrorCallback(device, device_error_cb, nullptr);

    // Get the device's queue
    WGPUQueue const queue = wgpuDeviceGetQueue(device);

    struct Context
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

        ~Context()
        {
            wgpuBufferRelease(src_buf);
            wgpuBufferRelease(dst_buf);
        }
    };
    
    Context ctx{};
    ctx.src_buf = make_buffer(device, 16, WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc);
    ctx.dst_buf = make_buffer(device, 16, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);

    // Submit command to copy data from one buffer to another
    {
        // Create command encoder
        WGPUCommandEncoder const encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
        auto const drop_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // Add command to the encoder
        wgpuCommandEncoderCopyBufferToBuffer(encoder, ctx.src_buf, 0, ctx.dst_buf, 0, 16);

        // Encode commands
        WGPUCommandBuffer const command = wgpuCommandEncoderFinish(encoder, nullptr);
        auto const drop_command = defer([=]() { wgpuCommandBufferRelease(command); });

        // Submit the encoded command
        wgpuQueueSubmit(queue, 1, &command);
    }

    // Read dst buffer data back to host asynchronously
    auto const map_cb = [](WGPUBufferMapAsyncStatus status, void* userdata)
    {
        auto ctx = static_cast<Context*>(userdata);

        if (status != WGPUBufferMapAsyncStatus_Success)
        {
            fmt::print("Failed to map dst buffer ({})\n", to_string(status));
            ctx->status = Context::Status_Failure;
        }
        else
        {
            fmt::print("Successfully mapped dst buffer\n");
            ctx->status = Context::Status_Success;

            // Ensure dst buffer is unmapped on scope exit
            auto const unmap_buf = defer([=]() { wgpuBufferUnmap(ctx->dst_buf); });

            // Print out the mapped buffer's data
            {
                using u8 = std::uint8_t;
                auto data =
                    static_cast<u8 const*>(wgpuBufferGetConstMappedRange(ctx->dst_buf, 0, 16));

                fmt::print("dst_buf = [{}", data[0]);
                for (int i = 1; i < 16; ++i)
                    fmt::print(", {}", data[i]);
                fmt::print("]\n", data[15]);
            }
        }
    };
    wgpuBufferMapAsync(ctx.dst_buf, WGPUMapMode_Read, 0, 16, map_cb, &ctx);

    // Poll device until async work is done
    while (ctx.Status_Pending)
        wgpu::poll_events(queue);

    return (ctx.status == Context::Status_Success) ? 0 : 1;
}
