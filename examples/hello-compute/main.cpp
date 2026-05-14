#include <cassert>

#include <type_traits>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include <dr/container_utils.hpp>
#include <dr/defer.hpp>
#include <dr/memory.hpp>
#include <dr/span.hpp>

#include <emsc_utils.hpp>

#include "shader_src.hpp"

#include "../gpu_context.hpp"
#include "../gpu_resource.hpp"

namespace wgpu::sandbox
{
namespace
{

struct UnaryKernel
{
    inline static GpuBindGroupLayout bind_group_layout{};
    inline static GpuPipelineLayout pipeline_layout{};

    GpuComputePipeline pipeline;
    GpuBindGroup bind_group;

    static void init_shared_resources(WGPUDevice const device)
    {
        bind_group_layout = make_bind_group_layout(device);
        pipeline_layout = make_pipeline_layout(device, bind_group_layout);
    }

    static UnaryKernel make(WGPUDevice const device, char const* const shader_src)
    {
        UnaryKernel result{};

        assert(pipeline_layout);
        result.pipeline = make_pipeline(device, pipeline_layout, {shader_src, WGPU_STRLEN});

        return result;
    }

    void update_bind_group(WGPUDevice const device, WGPUBuffer const buffer)
    {
        bind_group = make_bind_group(device, bind_group_layout, buffer);
        assert(bind_group);
    }

    void dispatch(WGPUComputePassEncoder const encoder)
    {
        wgpuComputePassEncoderSetPipeline(encoder, pipeline);
        wgpuComputePassEncoderSetBindGroup(encoder, 0, bind_group, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(encoder, 32, 1, 1);
    }

  private:
    static WGPUBindGroupLayout make_bind_group_layout(WGPUDevice const device)
    {
        WGPUBindGroupLayoutEntry const entries[]{
            {
                .binding = 0,
                .visibility = WGPUShaderStage_Compute,
                .buffer{.type = WGPUBufferBindingType_Storage},
            },
        };
        WGPUBindGroupLayoutDescriptor const desc{
            .entryCount = size(entries),
            .entries = entries,
        };
        return wgpuDeviceCreateBindGroupLayout(device, &desc);
    }

    static WGPUPipelineLayout make_pipeline_layout(
        WGPUDevice const device,
        WGPUBindGroupLayout const bind_layout)
    {
        WGPUPipelineLayoutDescriptor const desc{
            .bindGroupLayoutCount = 1,
            .bindGroupLayouts = &bind_layout,
        };
        return wgpuDeviceCreatePipelineLayout(device, &desc);
    }

    static WGPUComputePipeline make_pipeline(
        WGPUDevice const device,
        WGPUPipelineLayout const layout,
        WGPUStringView const shader_src)
    {
        WGPUShaderSourceWGSL shader_desc_src{
            .chain{.sType = WGPUSType_ShaderSourceWGSL},
            .code = shader_src,
        };
        WGPUShaderModuleDescriptor const shader_desc{
            .nextInChain = as<WGPUChainedStruct>(&shader_desc_src),
        };
        GpuShaderModule const shader = wgpuDeviceCreateShaderModule(device, &shader_desc);
        WGPUComputePipelineDescriptor const pipe_desc{
            .layout = layout,
            .compute{
                .module = shader,
                .entryPoint{"compute_main", WGPU_STRLEN},
            },
        };
        return wgpuDeviceCreateComputePipeline(device, &pipe_desc);
    }

    static WGPUBindGroup make_bind_group(
        WGPUDevice const device,
        WGPUBindGroupLayout const layout,
        WGPUBuffer const buffer)
    {
        WGPUBindGroupEntry const entries[]{
            {
                .binding = 0,
                .buffer = buffer,
                .size = wgpuBufferGetSize(buffer),
            },
        };
        WGPUBindGroupDescriptor const desc{
            .layout = layout,
            .entryCount = 1,
            .entries = entries,
        };
        return wgpuDeviceCreateBindGroup(device, &desc);
    }
};

struct ComputePass
{
    GpuComputePassEncoder encoder;

    static ComputePass make(WGPUCommandEncoder const cmd_encoder)
    {
        return {.encoder = wgpuCommandEncoderBeginComputePass(cmd_encoder, nullptr)};
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

struct
{
    GpuContext gpu;
    UnaryKernel kernel;
    GpuBuffer buffers[2]{};
} state;

WGPUBuffer make_buffer(WGPUDevice const device, size_t const size, WGPUBufferUsage const usage)
{
    WGPUBufferDescriptor const desc = {
        .usage = usage,
        .size = size,
    };
    return wgpuDeviceCreateBuffer(device, &desc);
}

void init()
{
    state.gpu = GpuContext::make();
    state.gpu.report();

    UnaryKernel::init_shared_resources(state.gpu.device);
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

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    init();

    state.kernel.update_bind_group(state.gpu.device, state.buffers[0]);

    // Dispatch command(s)
    {
        // Create command encoder
        GpuCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(
            state.gpu.device,
            nullptr);
        assert(cmd_encoder);

        // Compute pass
        {
            ComputePass pass = ComputePass::make(cmd_encoder);

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
        GpuCommandBuffer const cmds = wgpuCommandEncoderFinish(cmd_encoder, nullptr);
        assert(cmds);

        // Submit the encoded command
        WGPUQueue const queue = wgpuDeviceGetQueue(state.gpu.device);
        wgpuQueueSubmit(queue, 1, &cmds.handle());
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
