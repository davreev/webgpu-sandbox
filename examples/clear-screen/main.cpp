#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include "../example_app.hpp"
#include "../gpu_resource.hpp"

namespace wgpu::sandbox
{
namespace
{

using App = ExampleApp;

struct RenderPass
{
    GpuTextureView surface_view;
    GpuRenderPassEncoder encoder;

    static RenderPass make(WGPUCommandEncoder const cmd_encoder, WGPUSurface const surface)
    {
        RenderPass result{};

        result.surface_view = make_view(surface);
        assert(result.surface_view);

        result.encoder = begin(cmd_encoder, result.surface_view);
        assert(result.encoder);

        return result;
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
                .clearValue{1.0, 0.0, 0.5, 1.0},
            },
        };
        WGPURenderPassDescriptor const desc{
            .colorAttachmentCount = 1,
            .colorAttachments = color_atts,
        };
        return wgpuCommandEncoderBeginRenderPass(encoder, &desc);
    }
};

void update()
{
    GpuContext const& gpu = App::gpu();

    // Create a command encoder from the device
    GpuCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    assert(cmd_encoder);

    // Render pass
    {
        RenderPass pass = RenderPass::make(cmd_encoder, gpu.surface);

        // NOTE(dr): Render pass clears the screen by default
    }

    // Create encoded commands
    GpuCommandBuffer const cmds = wgpuCommandEncoderFinish(cmd_encoder, nullptr);
    assert(cmds);

    // Submit encoded commands
    WGPUQueue const queue = wgpuDeviceGetQueue(gpu.device);
    wgpuQueueSubmit(queue, 1, &cmds.handle());

    // Register callback that fires when queued work is done
    WGPUQueueWorkDoneCallbackInfo cb_info = {};
    cb_info.mode = WGPUCallbackMode_AllowSpontaneous;
    cb_info.callback =
#ifdef __EMSCRIPTEN__
        // NOTE(dr): Callback from webgpu.h in Emdawnwebgpu has a different signature
        [](WGPUQueueWorkDoneStatus const status,
           WGPUStringView /*msg*/,
           void* /*userdata1*/,
           void* /*userdata2*/) {
#else
        [](WGPUQueueWorkDoneStatus const status, void* /*userdata1*/, void* /*userdata2*/) {
#endif
            if (App::frame_count() % 100 == 0)
            {
                fmt::print(
                    "Finished frame {} with status: {}\n",
                    App::frame_count(),
                    to_string(status));
            }
        };
    wgpuQueueOnSubmittedWorkDone(queue, cb_info);
}

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    App::run({
        .frame_cb = update,
        .window{
            .title = "WebGPU Sandbox: Clear Screen",
            .width = 800,
            .height = 600,
        },
        .html_canvas_id = "#clear-screen",
    });

    return 0;
}
