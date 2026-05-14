#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include <dr/memory.hpp>

#include <dr/app/file_utils.hpp>

#include "../example_app.hpp"
#include "../gpu_resource.hpp"
#include "../passes.hpp"

namespace wgpu::sandbox
{
namespace
{

using App = ExampleApp;

struct
{
    GpuRenderPipeline pipeline{};
    // ...
} state;

WGPURenderPipeline make_render_pipeline(
    WGPUDevice const device,
    WGPUStringView const shader_src,
    WGPUTextureFormat const color_format)
{
    WGPUShaderSourceWGSL shader_desc_src{
        .chain = {.sType = WGPUSType_ShaderSourceWGSL},
        .code = shader_src,
    };
    WGPUShaderModuleDescriptor const shader_desc{
        .nextInChain = as<WGPUChainedStruct>(&shader_desc_src),
    };
    GpuShaderModule const shader = wgpuDeviceCreateShaderModule(device, &shader_desc);

    WGPUColorTargetState const color_targ{
        .format = color_format,
        // .blend = &(WGPUBlendState){
        // },
        .writeMask = WGPUColorWriteMask_All,
    };
    WGPUFragmentState const frag_state{
        .module = shader,
        .entryPoint = {"fs_main", WGPU_STRLEN},
        .targetCount = 1,
        .targets = &color_targ,
    };
    WGPURenderPipelineDescriptor const pipe_desc{
        .vertex{
            .module = shader,
            .entryPoint{"vs_main", WGPU_STRLEN},
        },
        .primitive{
            .topology = WGPUPrimitiveTopology_TriangleList,
            .frontFace = WGPUFrontFace_CCW,
            .cullMode = WGPUCullMode_None,
        },
        // .depthStencil = &(WGPUDepthStencilState){
        // },
        .multisample{
            .count = 1,
            .mask = ~0u,
            .alphaToCoverageEnabled = 0u,
        },
        .fragment = &frag_state,
    };
    return wgpuDeviceCreateRenderPipeline(device, &pipe_desc);
}

void init()
{
    String buffer;
    bool const read_ok = read_text_file("assets/shaders/triangle.wgsl", buffer);
    assert(read_ok);

    // Create render pipeline
    state.pipeline = make_render_pipeline(
        App::gpu().device,
        {buffer.c_str(), WGPU_STRLEN},
        default_surface_format);
}

void update()
{
    GpuContext const& gpu = App::gpu();

    // Create a command encoder from the device
    GpuCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    assert(cmd_encoder);

    // Render pass
    {
        auto pass = SurfaceRenderPass::make(cmd_encoder, gpu.surface);
        wgpuRenderPassEncoderSetPipeline(pass.encoder, state.pipeline);
        wgpuRenderPassEncoderDraw(pass.encoder, 3, 1, 0, 0);
    }

    // Create encoded commands
    GpuCommandBuffer const cmds = wgpuCommandEncoderFinish(cmd_encoder, nullptr);
    assert(cmds);

    // Submit encoded commands
    WGPUQueue const queue = wgpuDeviceGetQueue(gpu.device);
    wgpuQueueSubmit(queue, 1, &cmds.handle());
}

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    App::run({
        .init_cb = init,
        .frame_cb = update,
        .window{
            .title = "WebGPU Sandbox: Hello Triangle",
            .width = 800,
            .height = 600,
        },
        .html_canvas_id = "#hello-triangle",
    });

    return 0;
}
