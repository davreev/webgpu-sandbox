#include <cassert>

#include <fmt/core.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#include <webgpu/wgpu.h>
#endif

#include <webgpu/webgpu.h>

#include <dr/basic_types.hpp>
#include <dr/defer.hpp>

#include <wgpu_utils.hpp>

#include "../shared/dr_shim.hpp"
#include "utils.h"

namespace wgpu::sandbox
{
namespace
{

struct GpuContext
{
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;

    static GpuContext make()
    {
        GpuContext result{};

        // Create WebGPU instance
        result.instance = wgpuCreateInstance(nullptr);
        assert(result.instance);

        // Create WGPU adapter
        result.adapter = request_adapter(result.instance);
        assert(result.adapter);

        // Provide uncaptured error callback to device creation
        WGPUDeviceDescriptor device_desc = {};
        device_desc.uncapturedErrorCallbackInfo.callback = //
            [](WGPUDevice const* /*device*/,
               WGPUErrorType type,
               WGPUStringView msg,
               void* /*userdata1*/,
               void* /*userdata2*/) {
                fmt::print(
                    "WebGPU device error: {} ({})\nMessage: {}\n",
                    to_string(type),
                    int(type),
                    msg.data);
            };

        // Create WebGPU device
        result.device = request_device(result.adapter, &device_desc);
        assert(result.device);

        return result;
    }

    static void release(GpuContext& ctx)
    {
        wgpuDeviceRelease(ctx.device);
        wgpuAdapterRelease(ctx.adapter);
        wgpuInstanceRelease(ctx.instance);
        ctx = {};
    }
};

struct Kernel
{
    WGPUBuffer src_buf;
    WGPUBuffer dst_buf;

    static Kernel make(WGPUDevice const device, usize const buffer_size)
    {
        Kernel result{};

        result.src_buf = make_buffer(
            device,
            buffer_size,
            WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc);

        result.dst_buf = make_buffer(
            device,
            buffer_size,
            WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);

        return result;
    }

    static void release(Kernel& kernel)
    {
        wgpuBufferRelease(kernel.src_buf);
        wgpuBufferRelease(kernel.dst_buf);
        kernel = {};
    }

    void dispatch(WGPUCommandEncoder const cmd_encoder)
    {
        wgpuCommandEncoderCopyBufferToBuffer(
            cmd_encoder,
            src_buf,
            0,
            dst_buf,
            0,
            wgpuBufferGetSize(src_buf));
    }
};

struct AppState
{
    GpuContext gpu;
    Kernel kernel;
};

AppState state{};

void init_app()
{
    state.gpu = GpuContext::make();
    state.kernel = Kernel::make(state.gpu.device, 16);
}

void deinit_app()
{
    GpuContext::release(state.gpu);
    Kernel::release(state.kernel);
    state = {};
}

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    init_app();
    auto const _ = defer([]() { deinit_app(); });

    // Dispatch command(s)
    {
        // Create command encoder
        WGPUCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(
            state.gpu.device,
            nullptr);
        assert(cmd_encoder);
        auto const drop_cmd_encoder = defer([=]() { wgpuCommandEncoderRelease(cmd_encoder); });

        state.kernel.dispatch(cmd_encoder);

        // Create encoded commands
        WGPUCommandBuffer const cmds = wgpuCommandEncoderFinish(cmd_encoder, nullptr);
        assert(cmds);
        auto const drop_cmds = defer([=]() { wgpuCommandBufferRelease(cmds); });

        // Submit the encoded command
        WGPUQueue const queue = wgpuDeviceGetQueue(state.gpu.device);
        wgpuQueueSubmit(queue, 1, &cmds);
    }

    // Read dst buffer back to host asynchronously
    {
        auto const dst_buf = state.kernel.dst_buf;

        WGPUBufferMapCallbackInfo cb_info{};
        cb_info.userdata1 = dst_buf;
        cb_info.callback = //
            [](WGPUMapAsyncStatus status,
               WGPUStringView /*msg*/,
               void* userdata1,
               void* /*userdata2*/) {
                assert(status == WGPUMapAsyncStatus_Success);

                WGPUBuffer dst_buf = static_cast<WGPUBuffer>(userdata1);
                auto const unmap = defer([=]() { wgpuBufferUnmap(dst_buf); });

                // Print out the mapped buffer's data
                {
                    usize const size = wgpuBufferGetSize(dst_buf);
                    auto data = static_cast<u8 const*>(
                        wgpuBufferGetConstMappedRange(dst_buf, 0, size));

                    fmt::print("dst buf: [{}", data[0]);
                    for (usize i = 1; i < size; ++i)
                        fmt::print(", {}", data[i]);
                    fmt::print("]\n");
                }
#ifdef __EMSCRIPTEN__
                wgpu::raise_event("resultReady");
#endif
            };

        wgpuBufferMapAsync(dst_buf, WGPUMapMode_Read, 0, wgpuBufferGetSize(dst_buf), cb_info);

        // Wait until async work is done
#ifdef __EMSCRIPTEN__
        wgpu::wait_for_event("resultReady");
#else
        wgpuDevicePoll(state.gpu.device, true, nullptr);
#endif
    }

    return 0;
}
