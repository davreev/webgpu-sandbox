#include <cassert>

#include <fmt/core.h>

#include <GLFW/glfw3.h>

#include <webgpu/webgpu.h>

#include <wgpu_imgui.hpp>
#include <wgpu_utils.hpp>

#include "wgpu_config.h"

using namespace wgpu;

namespace
{

struct GpuContext
{
    WGPUInstance instance;
    WGPUSurface surface;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUTextureFormat surface_format;
    bool is_valid;
};

void config_surface(GpuContext& ctx, int width, int height)
{
    WGPUSurfaceConfiguration config{};
    {
        config.device = ctx.device;
        config.width = width;
        config.height = height;
        config.format = get_preferred_texture_format(ctx.surface, ctx.adapter);
        config.usage = WGPUTextureUsage_RenderAttachment;
    }

    ctx.surface_format = config.format; // Cache surface format
    wgpuSurfaceConfigure(ctx.surface, &config);
}

GpuContext make_gpu_context(GLFWwindow* window)
{
    GpuContext ctx{};

    // Create WebGPU instance
    ctx.instance = wgpuCreateInstance(nullptr);
    if (!ctx.instance)
    {
        fmt::print("Failed to create WebGPU instance\n");
        return ctx;
    }

    // Get WebGPU surface from GLFW window
    ctx.surface = make_surface(ctx.instance, window);
    if (!ctx.surface)
    {
        fmt::print("Failed to get WebGPU surface\n");
        return ctx;
    }

    // Create WGPU adapter
    WGPURequestAdapterOptions options{};
    {
        options.compatibleSurface = ctx.surface;
        options.powerPreference = WGPUPowerPreference_HighPerformance;
    }
    ctx.adapter = request_adapter(ctx.instance, &options);
    if (!ctx.adapter)
    {
        fmt::print("Failed to get WebGPU adapter\n");
        return ctx;
    }

    // Create WebGPU device
    ctx.device = request_device(ctx.adapter);
    if (!ctx.device)
    {
        fmt::print("Failed to get WebGPU device\n");
        return ctx;
    }

    // Set error callback on device
    wgpuDeviceSetUncapturedErrorCallback(
        ctx.device,
        [](WGPUErrorType type, char const* msg, void* /*userdata*/) {
            fmt::print(
                "WebGPU device error: {} ({})\nMessage: {}\n",
                to_string(type),
                static_cast<int>(type),
                msg);
        },
        nullptr);

    // Configure surface (replaces swap chain API)
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        config_surface(ctx, width, height);
    }

    ctx.is_valid = true;
    return ctx;
}

void release_gpu_context(GpuContext& ctx)
{
    wgpuSurfaceUnconfigure(ctx.surface);
    wgpuDeviceRelease(ctx.device);
    wgpuAdapterRelease(ctx.adapter);
    wgpuSurfaceRelease(ctx.surface);
    wgpuInstanceRelease(ctx.instance);
    ctx = {};
}

struct RenderPass
{
    WGPUTextureView view;
    WGPURenderPassEncoder encoder;
    bool is_valid;
};

RenderPass begin_render_pass(
    WGPUSurface const surface,
    WGPUCommandEncoder const encoder,
    WGPUColor const& clear_color)
{
    RenderPass pass{};

    WGPUSurfaceTexture const srf_tex = get_current_texture(surface);
    if (srf_tex.status != WGPUSurfaceGetCurrentTextureStatus_Success)
    {
        fmt::print("Failed to get surface texture ({})\n", to_string(srf_tex.status));
        return pass;
    }

    pass.view = make_texture_view(srf_tex);
    if (!pass.view)
    {
        fmt::print("Failed to create view of surface texture\n");
        return pass;
    }

    pass.encoder = begin_render_pass(encoder, pass.view, &clear_color);
    if (!pass.view)
    {
        fmt::print("Failed to create render pass encoder\n");
        return pass;
    }

    pass.is_valid = true;
    return pass;
}

void end_render_pass(RenderPass& pass)
{
    wgpuRenderPassEncoderEnd(pass.encoder);
    wgpuTextureViewRelease(pass.view);
    pass = {};
}

void init_imgui(GLFWwindow* window, GpuContext const& ctx)
{
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // ...
    }

    ImGui::StyleColorsDark();

    // Init GLFW impl
    ImGui_ImplGlfw_InitForOther(window, true);

    // Init WebGPU impl
    ImGui_ImplWGPU_InitInfo config{};
    {
        config.Device = ctx.device;
        config.NumFramesInFlight = 3;
        config.RenderTargetFormat = ctx.surface_format;
        config.DepthStencilFormat = WGPUTextureFormat_Undefined;
    }
    ImGui_ImplWGPU_Init(&config);
}

WGPUColor to_wgpu_color(float const c[3])
{
    return { c[0], c[1], c[2], 1.0 };
}

struct
{
    GpuContext gpu;
    float clear_color[3]{0.8f, 0.2f, 0.4f};
} state{};

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
            ImGui::TextWrapped("Minimal demo of ImGui with WebGPU/GLFW backend");
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    ImGui::Render();
}

} // namespace

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu;

    glfwSetErrorCallback(
        [](int errc, char const* msg) { fmt::print("GLFW error: {}\nMessage: {}\n", errc, msg); });

    // Initialize GLFW
    if (!glfwInit())
    {
        fmt::print("Failed to initialize GLFW\n");
        return 1;
    }
    auto const deinit_glfw = defer([]() { glfwTerminate(); });

    // Create GLFW window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* const window =
        glfwCreateWindow(800, 600, "WebGPU Sandbox: Hello ImGui", nullptr, nullptr);
    if (!window)
    {
        fmt::print("Failed to create window\n");
        return 1;
    }
    auto const drop_window = defer([=]() { glfwDestroyWindow(window); });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* /*window*/, int width, int height) {
        config_surface(state.gpu, width, height);
    });

    // Create WebGPU context
    state.gpu = make_gpu_context(window);
    if (!state.gpu.is_valid)
    {
        fmt::print("Failed to initialize WebGPU context\n");
        return 1;
    }
    auto const drop_gpu_ctx = defer([&]() { release_gpu_context(state.gpu); });

    // Initialize ImGui
    init_imgui(window, state.gpu);
    auto const deinit_imgui = defer([]() {
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    });

    // Get the device's default queue
    WGPUQueue const queue = wgpuDeviceGetQueue(state.gpu.device);

    // Frame loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // NOTE(dr): Use ImGuiIO::WantCapture* flags to determine if input events should be
        // forwarded to the main application. In general, when one of these flags is true, the
        // corresponding event should be consumed by ImGui.

        draw_ui();

        // Create a command encoder from the device
        WGPUCommandEncoder const encoder =
            wgpuDeviceCreateCommandEncoder(state.gpu.device, nullptr);
        auto const drop_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // Render pass
        {
            WGPUColor const clear_color = to_wgpu_color(state.clear_color);
            RenderPass pass = begin_render_pass(state.gpu.surface, encoder, clear_color);
            if (!pass.is_valid)
            {
                fmt::print("Failed to begin render pass\n");
                return 1;
            }
            auto const end_pass = defer([&]() { end_render_pass(pass); });

            // Issue ImGui draw commands
            ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.encoder);
        }

        // Create encoded commands
        WGPUCommandBuffer const commands = wgpuCommandEncoderFinish(encoder, nullptr);
        auto const drop_command = defer([=]() { wgpuCommandBufferRelease(commands); });

        // Submit encoded commands
        wgpuQueueSubmit(queue, 1, &commands);

        // Present the render target
        wgpuSurfacePresent(state.gpu.surface);
    }

    return 0;
}
