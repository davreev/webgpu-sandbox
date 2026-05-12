#include <cassert>

#include <fmt/core.h>

#include <imgui.h>

#include <webgpu/webgpu.h>

#include <dr/basic_types.hpp>

#include "../example_app.hpp"
#include "../gpu_resource.hpp"
#include "../surface_render_pass.hpp"

namespace wgpu::sandbox
{
namespace
{

using App = ExampleApp;

struct
{
    f32 clear_color[3]{0.8f, 0.2f, 0.4f};
    // ...
    // ...
    // ...
} state;

void draw_ui()
{
    App::ui_begin();

    ImGui::SetNextWindowPos({10.0f, 10.0f}, ImGuiCond_FirstUseEver);
    constexpr int window_flags = ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Hello ImGui", nullptr, window_flags);

    if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Settings"))
        {
            ImGui::ColorEdit3("Clear color", state.clear_color);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("About"))
        {
            ImGui::TextWrapped("Demo of ImGui with WebGPU/GLFW backend");
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    App::ui_end();
}

void update()
{
    GpuContext const& gpu = App::gpu();

    // NOTE(dr): Use ImGuiIO::WantCapture* flags to determine if input events should be
    // forwarded to the main application. In general, when one of these flags is true, the
    // corresponding event should be consumed by ImGui.
    draw_ui();

    // Create a command encoder from the device
    GpuCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    assert(cmd_encoder);
    // Render pass
    {
        constexpr auto to_wgpu_color = [](f32 const c[3]) -> WGPUColor {
            return {c[0], c[1], c[2], 1.0};
        };

        SurfaceRenderPass pass{
            cmd_encoder,
            gpu.surface,
            to_wgpu_color(state.clear_color)};

        // Issue UI draw command
        App::ui_draw(pass.encoder);
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
        .frame_cb = update,
        .window{
            .title = "WebGPU Sandbox: Hello ImGui",
            .width = 800,
            .height = 600,
        },
        .html_canvas_id = "#hello-imgui",
    });

    return 0;
}
