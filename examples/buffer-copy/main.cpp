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

#include "../dr_shim.hpp"
#include "shader_src.hpp"
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
        result.device = request_device(result.instance, result.adapter, &device_desc);
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

struct UnaryKernel
{
    WGPUBindGroupLayout bind_group_layout;
    WGPUBindGroup bind_group;
    WGPUPipelineLayout pipeline_layout;
    WGPUComputePipeline pipeline;

    static UnaryKernel make(WGPUDevice const device, char const* const shader_src)
    {
        UnaryKernel result{};

        result.bind_group_layout = unary_kernel_make_bind_group_layout(device);
        result.pipeline_layout = unary_kernel_make_pipeline_layout(device, result.bind_group_layout);
        result.pipeline = unary_kernel_make_pipeline(
            device,
            result.pipeline_layout,
            {shader_src, WGPU_STRLEN});

        return result;
    }

    static void release(UnaryKernel& kernel)
    {
        wgpuComputePipelineRelease(kernel.pipeline);
        wgpuBindGroupLayoutRelease(kernel.bind_group_layout);
        kernel = {};
    }

    void update_bind_group(WGPUDevice const device, WGPUBuffer const buffer)
    {
        if (bind_group)
            wgpuBindGroupRelease(bind_group);

        bind_group = unary_kernel_make_bind_group(device, bind_group_layout, buffer);
        assert(bind_group);
    }

    void dispatch(WGPUComputePassEncoder const encoder)
    {
        wgpuComputePassEncoderSetPipeline(encoder, pipeline);
        wgpuComputePassEncoderSetBindGroup(encoder, 0, bind_group, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(encoder, 32, 1, 1);
    }
};

struct ComputePass
{
    WGPUComputePassEncoder encoder;

    static ComputePass begin(WGPUCommandEncoder const cmd_encoder)
    {
        return {compute_pass_begin(cmd_encoder)};
    }

    static void end(ComputePass& pass)
    {
        wgpuComputePassEncoderEnd(pass.encoder);
        pass = {};
    }
};

struct AppState
{
    GpuContext gpu;
    UnaryKernel kernel;
    WGPUBuffer buffers[2];
};

AppState state{};

void init_app()
{
    state.gpu = GpuContext::make();
    state.kernel = UnaryKernel::make(state.gpu.device, shader_src);

    constexpr usize buffer_size = 100 * sizeof(f32);

    state.buffers[0] = make_buffer(
        state.gpu.device,
        buffer_size,
        WGPUBufferUsage_CopySrc | WGPUBufferUsage_Storage);

    state.buffers[1] = make_buffer(
        state.gpu.device,
        buffer_size,
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);
}

void deinit_app()
{
    wgpuBufferRelease(state.buffers[0]);
    wgpuBufferRelease(state.buffers[1]);
    UnaryKernel::release(state.kernel);
    GpuContext::release(state.gpu);
    state = {};
}

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    init_app();
    auto const _ = defer([]() { deinit_app(); });

    state.kernel.update_bind_group(state.gpu.device, state.buffers[0]);

    // Dispatch command(s)
    {
        // Create command encoder
        WGPUCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(
            state.gpu.device,
            nullptr);
        assert(cmd_encoder);
        auto const drop_cmd_encoder = defer([=]() { wgpuCommandEncoderRelease(cmd_encoder); });

        // Compute pass
        {
            ComputePass pass = ComputePass::begin(cmd_encoder);
            auto const end_pass = defer([&]() { ComputePass::end(pass); });

            // Dispatch compute kernels
            state.kernel.dispatch(pass.encoder);
            // ...
            // ...
            // ...
        }

        // Copy result to second buffer for read back
        wgpuCommandEncoderCopyBufferToBuffer(
            cmd_encoder,
            state.buffers[0],
            0,
            state.buffers[1],
            0,
            wgpuBufferGetSize(state.buffers[0]));

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
        auto const dst_buf = state.buffers[1];

        WGPUBufferMapCallbackInfo cb_info{};
        cb_info.userdata1 = dst_buf;
        cb_info.mode = WGPUCallbackMode_AllowSpontaneous;
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
                    usize const size = wgpuBufferGetSize(dst_buf) >> 2;
                    auto data = static_cast<f32 const*>(
                        wgpuBufferGetConstMappedRange(dst_buf, 0, size));

                    fmt::print("dst buf: [{}", data[0]);
                    for (usize i = 1; i < size; ++i)
                        fmt::print(", {}", data[i]);
                    fmt::print("]\n");
                }
#ifdef __EMSCRIPTEN__
                raise_event("resultReady");
#endif
            };

        [[maybe_unused]]
        WGPUFuture const fut =
            wgpuBufferMapAsync(dst_buf, WGPUMapMode_Read, 0, wgpuBufferGetSize(dst_buf), cb_info);

        // Wait until async work is done
#ifdef __EMSCRIPTEN__
        wait_for_event("resultReady");
#else
        // TODO(dr): Check if async handling is actually required with wgpu-native
        // wait_for_future(instance, fut);

        // NOTE(dr): This seems to be necessary
        wgpuDevicePoll(state.gpu.device, true, nullptr);
        
        // TODO(dr): Use standard process events instead
        // wgpuInstanceProcessEvents(state.gpu.instance);
#endif
    }

    return 0;
}
