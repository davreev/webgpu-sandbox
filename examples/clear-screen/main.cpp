#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include <dr/basic_types.hpp>
#include <dr/defer.hpp>

#include <emsc_utils.hpp>
#include <wgpu_utils.hpp>

#include "graphics.h"

#include "../example_base.hpp"

namespace wgpu::sandbox
{
namespace
{

struct RenderPass
{
    WGPURenderPassEncoder encoder;
    WGPUTextureView surface_view;

    static RenderPass begin(WGPUCommandEncoder const cmd_encoder, WGPUSurface const surface)
    {
        RenderPass result{};

        result.surface_view = surface_make_view(surface);
        assert(result.surface_view);

        result.encoder = render_pass_begin(cmd_encoder, result.surface_view);
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

struct AppState
{
    GLFWwindow* window;
    GpuContext gpu;
    usize frame_count;
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
        "WebGPU Sandbox: Clear Screen",
        nullptr,
        nullptr);
    assert(state.window);

    // Create WebGPU context and report details
    state.gpu = GpuContext::make({state.window, "#clear-screen"});
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
}

void deinit_app()
{
    GpuContext::release(state.gpu);
    glfwDestroyWindow(state.window);
    glfwTerminate();
    state = {};
}

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    init_app();
    auto const _ = defer([]() { deinit_app(); });

    // Main loop body
    constexpr auto loop_body = []() {
        glfwPollEvents();

        // Create a command encoder from the device
        WGPUCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(
            state.gpu.device,
            nullptr);
        assert(cmd_encoder);
        auto const drop_cmd_encoder = defer([=]() { wgpuCommandEncoderRelease(cmd_encoder); });

        // Render pass
        {
            RenderPass pass = RenderPass::begin(cmd_encoder, state.gpu.surface);
            auto const end_pass = defer([&]() { RenderPass::end(pass); });

            // NOTE(dr): Render pass clears the screen by default
        }

        // Create encoded commands
        WGPUCommandBuffer const cmds = wgpuCommandEncoderFinish(cmd_encoder, nullptr);
        assert(cmds);
        auto const drop_cmds = defer([=]() { wgpuCommandBufferRelease(cmds); });

        // Submit encoded commands
        WGPUQueue const queue = wgpuDeviceGetQueue(state.gpu.device);
        wgpuQueueSubmit(queue, 1, &cmds);

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
                if (state.frame_count % 100 == 0)
                {
                    fmt::print(
                        "Finished frame {} with status: {}\n",
                        state.frame_count,
                        to_string(status));
                }
                ++state.frame_count;
            };
        wgpuQueueOnSubmittedWorkDone(queue, cb_info);
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
