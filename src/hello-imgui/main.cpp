#include <cassert>

#include <fmt/core.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <dr/basic_types.hpp>
#include <dr/defer.hpp>

#include <webgpu/webgpu.h>

#include <wgpu_imgui.hpp>
#include <wgpu_utils.hpp>

#include "../shared/dr_shim.hpp"
#include "graphics.h"

namespace wgpu::sandbox
{
namespace
{

struct GpuContext
{
    WGPUInstance instance;
    WGPUSurface surface;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUTextureFormat surface_format;

    static GpuContext make(GLFWwindow* const window)
    {
        GpuContext result{};

        // Create WebGPU instance
        result.instance = wgpuCreateInstance(nullptr);
        assert(result.instance);

#ifdef __EMSCRIPTEN__
        // Get WebGPU surface from the HTML canvas
        result.surface = make_surface(result.instance, "#hello-imgui");
#else
        // Get WebGPU surface from GLFW window
        result.surface = make_surface(result.instance, window);
#endif
        assert(result.surface);

        // Create WGPU adapter
        WGPURequestAdapterOptions options{};
        {
            options.compatibleSurface = result.surface;
            options.powerPreference = WGPUPowerPreference_HighPerformance;
        }
        result.adapter = request_adapter(result.instance, &options);
        assert(result.adapter);

        // Create WebGPU device
        result.device = request_device(result.adapter);
        assert(result.device);

        // Set error callback on device
        wgpuDeviceSetUncapturedErrorCallback(
            result.device,
            [](WGPUErrorType type, char const* msg, void* /*userdata*/) {
                fmt::print(
                    "WebGPU device error: {} ({})\nMessage: {}\n",
                    to_string(type),
                    static_cast<int>(type),
                    msg);
            },
            nullptr);

        result.surface_format = get_preferred_texture_format(result.surface, result.adapter);
        result.config_surface(window);

        return result;
    }

    static void release(GpuContext& ctx)
    {
        wgpuSurfaceUnconfigure(ctx.surface);
        wgpuDeviceRelease(ctx.device);
        wgpuAdapterRelease(ctx.adapter);
        wgpuSurfaceRelease(ctx.surface);
        wgpuInstanceRelease(ctx.instance);
        ctx = {};
    }

    void config_surface(int const width, int const height)
    {
        WGPUSurfaceConfiguration config{};
        {
            config.device = device;
            config.width = width;
            config.height = height;
            config.format = surface_format;
            config.usage = WGPUTextureUsage_RenderAttachment;
#ifdef __EMSCRIPTEN__
            // NOTE(dr): Default value from Emscripten's webgpu.h is undefined
            config.presentMode = WGPUPresentMode_Fifo;
#endif
        }
        wgpuSurfaceConfigure(surface, &config);
    }

    void config_surface(GLFWwindow* const window)
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        config_surface(width, height);
    }
};

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
            config.RenderTargetFormat = gpu.surface_format;
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
    wgpu::get_canvas_client_size(init_width, init_height);
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

    // Create WebGPU context
    state.gpu = GpuContext::make(state.window);

#ifdef __EMSCRIPTEN__
    // Handle canvas resize
    auto constexpr resize_cb =
        [](int /*event_type*/, EmscriptenUiEvent const* /*event*/, void* /*userdata*/) -> bool {
        int w, h;
        wgpu::get_canvas_client_size(w, h);
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
    constexpr auto loop_body = []() {
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

    // Main loop
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop_body, 0, true);
#else
    while (!glfwWindowShouldClose(state.window))
    {
        loop_body();
        wgpuSurfacePresent(state.gpu.surface);
    }
#endif

    return 0;
}
