#include <cassert>

#include <fmt/core.h>

#include <imgui.h>

#include <webgpu/webgpu.h>

#include <dr/basic_types.hpp>
#include <dr/defer.hpp>

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

    static RenderPass begin(
        WGPUCommandEncoder const cmd_encoder,
        WGPUSurface const surface,
        WGPUColor const& clear_color)
    {
        RenderPass result{};

        result.surface_view = make_view(surface);
        assert(result.surface_view);

        result.encoder = begin(cmd_encoder, result.surface_view, clear_color);
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
        WGPUTextureView const surface_view,
        WGPUColor const& clear_color)
    {
        WGPURenderPassColorAttachment color_atts[]{
            {
                .view = surface_view,
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue = clear_color,
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
    WGPUCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    assert(cmd_encoder);
    auto const drop_cmd_encoder = defer([=]() { wgpuCommandEncoderRelease(cmd_encoder); });

    // Render pass
    {
        constexpr auto to_wgpu_color = [](f32 const c[3]) -> WGPUColor {
            return {c[0], c[1], c[2], 1.0};
        };

        RenderPass pass = RenderPass::begin(
            cmd_encoder,
            gpu.surface,
            to_wgpu_color(state.clear_color));
        auto const end_pass = defer([&]() { RenderPass::end(pass); });

        // Issue UI draw command
        App::ui_draw(pass.encoder);
    }

    // Create encoded commands
    WGPUCommandBuffer const cmds = wgpuCommandEncoderFinish(cmd_encoder, nullptr);
    assert(cmds);
    auto const drop_cmds = defer([=]() { wgpuCommandBufferRelease(cmds); });

    // Submit encoded commands
    WGPUQueue const queue = wgpuDeviceGetQueue(gpu.device);
    wgpuQueueSubmit(queue, 1, &cmds);
}

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    App::init({
        .frame_cb = update,
        .window{
            .title = "WebGPU Sandbox: Hello ImGui",
            .width = 800,
            .height = 600,
        },
        .html_canvas_id = "#hello-imgui",
    });
    App::run();
    App::deinit();

    return 0;
}
