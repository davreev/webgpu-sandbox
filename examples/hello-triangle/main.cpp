#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include <dr/basic_types.hpp>
#include <dr/defer.hpp>
#include <dr/memory.hpp>

#include <dr/app/file_utils.hpp>

#include "../example_app.hpp"

namespace wgpu::sandbox
{
namespace
{

using App = ExampleApp;

struct RenderPass
{
    WGPURenderPassEncoder encoder;
    WGPUTextureView surface_view;

    static RenderPass begin(WGPUCommandEncoder const cmd_encoder, WGPUSurface const surface)
    {
        RenderPass result{};

        result.surface_view = make_view(surface);
        assert(result.surface_view);

        result.encoder = begin(cmd_encoder, result.surface_view);
        assert(result.encoder);

        return result;
    }

    static void end(RenderPass& pass)
    {
        wgpuRenderPassEncoderEnd(pass.encoder);
        wgpuRenderPassEncoderRelease(pass.encoder);
        wgpuTextureViewRelease(pass.surface_view);
        pass = {};
    }

  private:
    static WGPUTextureView make_view(WGPUSurface const surface)
    {
        WGPUSurfaceTexture srf_tex;
        wgpuSurfaceGetCurrentTexture(surface, &srf_tex);
        assert(srf_tex.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal);

        WGPUTextureViewDescriptor const desc{
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
        };
        return wgpuTextureCreateView(srf_tex.texture, &desc);
    }

    static WGPURenderPassEncoder begin(
        WGPUCommandEncoder const encoder,
        WGPUTextureView const surface_view)
    {
        WGPURenderPassColorAttachment color_atts[]{
            {
                .view = surface_view,
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue{0.15, 0.15, 0.15, 1.0},
            },
        };
        WGPURenderPassDescriptor const desc{
            .colorAttachmentCount = 1,
            .colorAttachments = color_atts,
        };
        return wgpuCommandEncoderBeginRenderPass(encoder, &desc);
    }
};

struct
{
    WGPURenderPipeline pipeline{};
    // ...
    // ...
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
    WGPUShaderModule const shader = wgpuDeviceCreateShaderModule(device, &shader_desc);
    auto const drop_shader = defer([=]() { wgpuShaderModuleRelease(shader); });

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
    WGPUCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    assert(cmd_encoder);
    auto const drop_cmd_encoder = defer([=]() { wgpuCommandEncoderRelease(cmd_encoder); });

    // Render pass
    {
        RenderPass pass = RenderPass::begin(cmd_encoder, gpu.surface);
        auto const end_pass = defer([&]() { RenderPass::end(pass); });

        wgpuRenderPassEncoderSetPipeline(pass.encoder, state.pipeline);
        wgpuRenderPassEncoderDraw(pass.encoder, 3, 1, 0, 0);
    }

    // Create encoded commands
    WGPUCommandBuffer const cmds = wgpuCommandEncoderFinish(cmd_encoder, nullptr);
    assert(cmds);
    auto const drop_cmds = defer([=]() { wgpuCommandBufferRelease(cmds); });

    // Submit encoded commands
    WGPUQueue const queue = wgpuDeviceGetQueue(gpu.device);
    wgpuQueueSubmit(queue, 1, &cmds);
}

void deinit()
{
    wgpuRenderPipelineRelease(state.pipeline);
    state = {};
}

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    App::init({
        .init_cb = init,
        .frame_cb = update,
        .deinit_cb = deinit,
        .window{
            .title = "WebGPU Sandbox: Hello Triangle",
            .width = 800,
            .height = 600,
        },
        .html_canvas_id = "#hello-triangle",
    });
    App::run();
    App::deinit();

    return 0;
}
