#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include "../example_app.hpp"
#include "../gpu_resource.hpp"
#include "../surface_render_pass.hpp"

namespace wgpu::sandbox
{
namespace
{

using App = ExampleApp;

void update()
{
    GpuContext const& gpu = App::gpu();

    // Create a command encoder from the device
    GpuCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    assert(cmd_encoder);

    // Render pass
    {
        SurfaceRenderPass pass{cmd_encoder, gpu.surface, {1.0, 0.0, 0.5, 1.0}};

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
