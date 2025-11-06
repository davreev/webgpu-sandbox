#include <cassert>

#include <type_traits>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include <dr/basic_types.hpp>
#include <dr/defer.hpp>
#include <dr/memory.hpp>
#include <dr/span.hpp>

#include <emsc_utils.hpp>
#include <wgpu_utils.hpp>

#include "config.h"
#include "shader_src.hpp"

#include "../example_base.hpp"

namespace wgpu::sandbox
{
namespace
{

struct UnaryKernel
{
    inline static WGPUBindGroupLayout bind_group_layout{};
    inline static WGPUPipelineLayout pipeline_layout{};

    WGPUComputePipeline pipeline;
    WGPUBindGroup bind_group;

    static void init(WGPUDevice const device)
    {
        bind_group_layout = unary_kernel_make_bind_group_layout(device);
        pipeline_layout = unary_kernel_make_pipeline_layout(device, bind_group_layout);
    }

    static void deinit()
    {
        wgpuPipelineLayoutRelease(pipeline_layout);
        pipeline_layout = {};

        wgpuBindGroupLayoutRelease(bind_group_layout);
        bind_group_layout = {};
    }

    static UnaryKernel make(WGPUDevice const device, char const* const shader_src)
    {
        UnaryKernel result{};

        assert(pipeline_layout);
        result.pipeline = unary_kernel_make_pipeline(
            device,
            pipeline_layout,
            {shader_src, WGPU_STRLEN});

        return result;
    }

    static void release(UnaryKernel& kernel)
    {
        if (kernel.bind_group)
            wgpuBindGroupRelease(kernel.bind_group);

        wgpuComputePipelineRelease(kernel.pipeline);

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
        wgpuComputePassEncoderRelease(pass.encoder);
        pass = {};
    }
};

template <typename Action>
void read_buffer(
    [[maybe_unused]] WGPUInstance const instance,
    WGPUBuffer const buffer,
    Action&& action)
{
    static_assert(std::is_invocable_v<Action, Span<u8 const>>);

    struct MapResult
    {
        WGPUBuffer buffer;
        bool is_ready;
    } result{buffer, false};

    WGPUBufferMapCallbackInfo cb_info{};
    cb_info.userdata1 = &result;
    cb_info.userdata2 = &action;
    cb_info.mode = WGPUCallbackMode_AllowSpontaneous;
    cb_info.callback = //
        [](WGPUMapAsyncStatus status, WGPUStringView /*msg*/, void* userdata1, void* userdata2) {
            auto& result = *static_cast<MapResult*>(userdata1);
            auto& action = *static_cast<Action*>(userdata2);

            // If map was successful, need to unmap the buffer when we're done here
            assert(status == WGPUMapAsyncStatus_Success);
            auto const unmap = defer([&]() { wgpuBufferUnmap(result.buffer); });

            // Perform some action on the buffer contents
            usize const size = wgpuBufferGetSize(result.buffer);
            u8 const* bytes = as<u8>(wgpuBufferGetConstMappedRange(result.buffer, 0, size));
            action(Span(bytes, size));

#ifdef __EMSCRIPTEN__
            raise_event("resultReady");
#else
            result.is_ready = true;
#endif
        };

    [[maybe_unused]]
    WGPUFuture const fut = wgpuBufferMapAsync(
        result.buffer,
        WGPUMapMode_Read,
        0,
        wgpuBufferGetSize(result.buffer),
        cb_info);

    // Wait until async work is done
#ifdef __EMSCRIPTEN__
    wait_for_event("resultReady");
#else
    // NOTE(dr): Waiting on futures is not yet implemented in wgpu-native
    // wait_for_future(instance, fut);
    wait_for_condition(instance, [&]() { return result.is_ready; });
#endif
}

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
    state.gpu.report();

    UnaryKernel::init(state.gpu.device);
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
    UnaryKernel::deinit();
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

    // Read second buffer back and print out values
    read_buffer(state.gpu.instance, state.buffers[1], [](Span<u8 const> data) {
        auto vals = as<f32>(data);
        fmt::print("buffer: [{}", data[0]);
        for (f32 const val : vals)
            fmt::print(", {}", val);
        fmt::print("]\n");
    });

    return 0;
}
