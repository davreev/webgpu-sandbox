#include <cassert>

#include <fmt/core.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <GLFW/glfw3.h>

#include <webgpu/webgpu.h>

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
#ifdef __EMSCRIPTEN__
        // NOTE(dr): Default value from Emscripten's webgpu.h is undefined
        config.presentMode = WGPUPresentMode_Fifo;
#endif
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

#ifdef __EMSCRIPTEN__
    // Get WebGPU surface from the HTML canvas
    ctx.surface = make_surface(ctx.instance, "#clear-screen");
#else
    // Get WebGPU surface from GLFW window
    ctx.surface = make_surface(ctx.instance, window);
#endif
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

    // Configure surface
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
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

RenderPass begin_render_pass(WGPUSurface const surface, WGPUCommandEncoder const encoder)
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

    pass.encoder = begin_render_pass(encoder, pass.view);
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

struct State
{
    GLFWwindow* window;
    GpuContext gpu;
    WGPURenderPipeline pipeline;
    WGPUQueue queue;
    std::size_t frame_count;
};

State state{};

} // namespace

int main(int /*argc*/, char** /*argv*/)
{
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
#ifdef __EMSCRIPTEN__
    int init_width, init_height;
    get_canvas_client_size(init_width, init_height);
#else
    constexpr int init_width = 800;
    constexpr int init_height = 600;
#endif
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    state.window =
        glfwCreateWindow(init_width, init_height, "WebGPU Sandbox: Clear Screen", nullptr, nullptr);
    if (!state.window)
    {
        fmt::print("Failed to create window\n");
        return 1;
    }
    auto const drop_window = defer([]() { glfwDestroyWindow(state.window); });

    // Create WebGPU context
    state.gpu = make_gpu_context(state.window);
    if (!state.gpu.is_valid)
    {
        fmt::print("Failed to initialize WebGPU context\n");
        return 1;
    }
    auto const drop_gpu = defer([]() { release_gpu_context(state.gpu); });

#ifdef __EMSCRIPTEN__
    // Handle canvas resize
    emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,
        nullptr,
        false,
        [](int /*event_type*/, EmscriptenUiEvent const* /*event*/, void* /*userdata*/) -> bool {
            int new_size[2];
            get_canvas_client_size(new_size[0], new_size[1]);
            glfwSetWindowSize(state.window, new_size[0], new_size[1]);
            return true;
        });
#else
    // Handle window resize
    glfwSetFramebufferSizeCallback(state.window, [](GLFWwindow* /*window*/, int width, int height) {
        config_surface(state.gpu, width, height);
    });
#endif

// NOTE(dr): Report utils are only compatible with wgpu-native for now
#ifndef __EMSCRIPTEN__
    report_adapter_features(state.gpu.adapter);
    report_adapter_limits(state.gpu.adapter);
    report_adapter_properties(state.gpu.adapter);
    report_device_features(state.gpu.device);
    report_device_limits(state.gpu.device);
    report_surface_capabilities(state.gpu.surface, state.gpu.adapter);
#endif

    // Cache the device's queue
    state.queue = wgpuDeviceGetQueue(state.gpu.device);

    // Main loop body
    constexpr auto loop_body = []() {
        glfwPollEvents();

        // Create a command encoder from the device
        WGPUCommandEncoder const encoder =
            wgpuDeviceCreateCommandEncoder(state.gpu.device, nullptr);
        auto const drop_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // NOTE(dr): Can use debug markers on the encoder for printf-like debugging bw commands
        // wgpuCommandEncoderInsertDebugMarker(encoder, "Do a thing");
        // wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

        // Render pass
        {
            RenderPass pass = begin_render_pass(state.gpu.surface, encoder);
            assert(pass.is_valid);
            auto const end_pass = defer([&]() { end_render_pass(pass); });

            // NOTE(dr): Render pass clears the screen by default
        }

        // Create a command via the encoder
        WGPUCommandBuffer const command = wgpuCommandEncoderFinish(encoder, nullptr);
        auto const drop_command = defer([=]() { wgpuCommandBufferRelease(command); });

        // Submit encoded command
        wgpuQueueSubmit(state.queue, 1, &command);

        // Register callback that fires when queued work is done
        constexpr auto work_done_cb = [](WGPUQueueWorkDoneStatus const status, void* /*userdata*/) {
            if (state.frame_count % 100 == 0)
            {
                fmt::print(
                    "Finished frame {} with status: {}\n",
                    state.frame_count,
                    to_string(status));
            }
            ++state.frame_count;
        };
        wgpuQueueOnSubmittedWorkDone(state.queue, work_done_cb, nullptr);
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
