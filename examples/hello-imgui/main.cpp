#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include <dr/basic_types.hpp>
#include <dr/defer.hpp>

#include <emsc_utils.hpp>
#include <wgpu_imgui.hpp>
#include <wgpu_utils.hpp>

#include "config.h"

#include "../example_base.hpp"

namespace wgpu::sandbox
{
namespace
{

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

        result.surface_view = surface_make_view(surface);
        assert(result.surface_view);

        result.encoder = render_pass_begin(cmd_encoder, result.surface_view, &clear_color);
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
};

struct UI
{
    static void init(GLFWwindow* window, GpuContext const& gpu)
    {
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        {
            io.IniFilename = nullptr;
            io.LogFilename = nullptr;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            // ...
        }

        ImGui::StyleColorsDark();

        // Init GLFW impl
        ImGui_ImplGlfw_InitForOther(window, true);

        // Init WebGPU impl
        ImGui_ImplWGPU_InitInfo config{};
        {
            config.Device = gpu.device;
            config.NumFramesInFlight = 3;
            config.RenderTargetFormat = default_surface_format;
            config.DepthStencilFormat = WGPUTextureFormat_Undefined;
        }
        ImGui_ImplWGPU_Init(&config);
    }

    static void deinit()
    {
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    static void dispatch_draw(WGPURenderPassEncoder const encoder)
    {
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), encoder);
    }
};

struct AppState
{
    GLFWwindow* window;
    GpuContext gpu;
    float clear_color[3]{0.8f, 0.2f, 0.4f};
};

AppState state{};

void init_app()
{
    glfwSetErrorCallback(
        [](int errc, char const* msg) { fmt::print("GLFW error: {}\nMessage: {}\n", errc, msg); });

    // Initialize GLFW
    bool const glfw_ok = glfwInit();
    assert(glfw_ok);

    // Create GLFW window
#ifdef __EMSCRIPTEN__
    int init_width, init_height;
    get_canvas_client_size(init_width, init_height);
#else
    constexpr int init_width = 800;
    constexpr int init_height = 600;
#endif
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    state.window = glfwCreateWindow(
        init_width,
        init_height,
        "WebGPU Sandbox: Hello ImGui",
        nullptr,
        nullptr);
    assert(state.window);

    // Create WebGPU context and report details
    state.gpu = GpuContext::make({state.window, "#hello-imgui"});
    state.gpu.report();

#ifdef __EMSCRIPTEN__
    // Handle canvas resize
    auto constexpr resize_cb =
        [](int /*event_type*/, EmscriptenUiEvent const* /*event*/, void* /*userdata*/) -> bool {
        int w, h;
        get_canvas_client_size(w, h);
        glfwSetWindowSize(state.window, w, h);
        return true;
    };
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, false, resize_cb);
#else
    // Handle framebuffer resize
    glfwSetFramebufferSizeCallback(state.window, [](GLFWwindow* /*window*/, int width, int height) {
        state.gpu.config_surface(width, height);
    });
#endif

    UI::init(state.window, state.gpu);
}

void deinit_app()
{
    UI::deinit();
    GpuContext::release(state.gpu);
    glfwDestroyWindow(state.window);
    glfwTerminate();
    state = {};
}

void draw_ui()
{
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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
    ImGui::Render();
}

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    init_app();
    auto const _ = defer([]() { deinit_app(); });

    // Main loop
    constexpr auto loop_cb = [](void* /*userdata*/) {
        glfwPollEvents();

        // NOTE(dr): Use ImGuiIO::WantCapture* flags to determine if input events should be
        // forwarded to the main application. In general, when one of these flags is true, the
        // corresponding event should be consumed by ImGui.

        draw_ui();

        // Create a command encoder from the device
        WGPUCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(
            state.gpu.device,
            nullptr);
        assert(cmd_encoder);
        auto const drop_cmd_encoder = defer([=]() { wgpuCommandEncoderRelease(cmd_encoder); });

        // Render pass
        {
            constexpr auto to_wgpu_color = [](float const c[3]) -> WGPUColor {
                return {c[0], c[1], c[2], 1.0};
            };

            RenderPass pass = RenderPass::begin(
                cmd_encoder,
                state.gpu.surface,
                to_wgpu_color(state.clear_color));
            auto const end_pass = defer([&]() { RenderPass::end(pass); });

            // Issue UI draw command
            UI::dispatch_draw(pass.encoder);
        }

        // Create encoded commands
        WGPUCommandBuffer const cmds = wgpuCommandEncoderFinish(cmd_encoder, nullptr);
        assert(cmds);
        auto const drop_cmds = defer([=]() { wgpuCommandBufferRelease(cmds); });

        // Submit encoded commands
        WGPUQueue const queue = wgpuDeviceGetQueue(state.gpu.device);
        wgpuQueueSubmit(queue, 1, &cmds);
    };

    MainLoop{state.gpu.surface, state.window, loop_cb}.begin();

    return 0;
}
