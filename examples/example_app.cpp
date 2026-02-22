#include "example_app.hpp"

#include <cassert>

#include <fmt/core.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include <emsc_utils.hpp>
#include <wgpu_imgui.hpp>

namespace wgpu::sandbox
{
namespace
{

void ui_init(GLFWwindow* window, GpuContext const& ctx)
{
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // ...

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOther(window, true);

    ImGui_ImplWGPU_InitInfo info{};
    info.Device = ctx.device;
    info.NumFramesInFlight = 3;
    info.RenderTargetFormat = default_surface_format;
    info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    ImGui_ImplWGPU_Init(&info);
}

void ui_deinit()
{
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

struct
{
    ExampleApp::Callback* frame_cb{};
    ExampleApp::Callback* deinit_cb{};
    ExampleApp::EventCallback* event_cb{};
    void* userdata{};
    GLFWwindow* window{};
    GpuContext gpu{};
    usize frame_count{};
} state;

void set_event_callbacks()
{
    using Event = ExampleApp::Event;

    static constexpr auto handle_event = [](Event const& e) {
        if (state.event_cb)
            state.event_cb(e);
    };

    glfwSetKeyCallback(
        state.window,
        [](GLFWwindow* /*window*/, i32 key, i32 scancode, i32 action, i32 mods) {
            handle_event({
                .type = Event::Type::Key,
                .key{
                    .key = key,
                    .scancode = scancode,
                    .action = action,
                    .mods = mods,
                },
            });
        });

    glfwSetMouseButtonCallback(
        state.window,
        [](GLFWwindow* /*window*/, i32 button, i32 action, i32 mods) {
            handle_event({
                .type = Event::Type::MouseButton,
                .mouse_button{
                    .button = button,
                    .action = action,
                    .mods = mods,
                },
            });
        });

    glfwSetScrollCallback(state.window, [](GLFWwindow* /*window*/, f64 d_x, f64 d_y) {
        handle_event({
            .type = Event::Type::Scroll,
            .scroll{
                .offset{d_x, d_y},
            },
        });
    });

    glfwSetCursorPosCallback(state.window, [](GLFWwindow* /*window*/, f64 p_x, f64 p_y) {
        handle_event({
            .type = Event::Type::CursorMove,
            .cursor_move{
                .position{p_x, p_y},
            },
        });
    });

    glfwSetCursorEnterCallback(state.window, [](GLFWwindow* /*window*/, i32 entered) {
        handle_event({
            .type = Event::Type::CursorEnter,
            .cursor_enter{
                .entered = (entered == GLFW_TRUE),
            },
        });
    });

    glfwSetWindowFocusCallback(state.window, [](GLFWwindow* /*window*/, i32 focused) {
        handle_event({
            .type = Event::Type::WindowFocus,
            .window_focus{
                .focused = (focused == GLFW_TRUE),
            },
        });
    });

    // Handle framebuffer resize
    glfwSetFramebufferSizeCallback(state.window, [](GLFWwindow* /*window*/, i32 width, i32 height) {
#ifndef __EMSCRIPTEN__
        state.gpu.config_surface(width, height);
#endif
        handle_event({
            .type = Event::Type::FramebufferResize,
            .framebuffer_resize{
                .width = width,
                .height = height,
            },
        });
    });

#ifdef __EMSCRIPTEN__
    // Handle HTML canvas resize (triggers framebuffer resize)
    auto constexpr resize_cb =
        [](i32 /*event_type*/, EmscriptenUiEvent const* /*event*/, void* /*userdata*/) -> bool {
        i32 w, h;
        get_canvas_client_size(w, h);
        glfwSetWindowSize(state.window, w, h);
        return true;
    };
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, false, resize_cb);
#endif
}

} // namespace

void ExampleApp::init(Desc const& desc)
{
    assert(desc.frame_cb);

    state.frame_cb = desc.frame_cb;
    state.deinit_cb = desc.deinit_cb;
    state.event_cb = desc.event_cb;
    state.userdata = desc.userdata;

    glfwSetErrorCallback(
        [](int errc, char const* msg) { fmt::print("GLFW error: {}\nMessage: {}\n", errc, msg); });

    bool const glfw_ok = glfwInit();
    assert(glfw_ok);

    i32 width = desc.window.width;
    i32 height = desc.window.height;
#ifdef __EMSCRIPTEN__
    get_canvas_client_size(width, height);
#endif
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    state.window = glfwCreateWindow(width, height, desc.window.title, nullptr, nullptr);
    assert(state.window);

    state.gpu = GpuContext::make({
        .window = state.window,
        .canvas_id = desc.html_canvas_id,
    });
    state.gpu.report();

    set_event_callbacks();

    ui_init(state.window, state.gpu);

    if (desc.init_cb)
        desc.init_cb();
}

void ExampleApp::run()
{
    assert(state.window);

    static constexpr auto main_loop = []() {
        glfwPollEvents();
        state.frame_cb();
        ++state.frame_count;
    };

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, true);
#else
    while (!glfwWindowShouldClose(state.window))
    {
        main_loop();
        wgpuSurfacePresent(state.gpu.surface);
    }
#endif
}

void ExampleApp::deinit()
{
    assert(state.window);

    if (state.deinit_cb)
        state.deinit_cb();

    ui_deinit();
    GpuContext::release(state.gpu);
    glfwDestroyWindow(state.window);
    glfwTerminate();
    state = {};
}

void ExampleApp::ui_begin()
{
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ExampleApp::ui_end() { ImGui::Render(); }

void ExampleApp::ui_draw(WGPURenderPassEncoder const encoder)
{
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), encoder);
}

void* ExampleApp::userdata() { return state.userdata; }

GLFWwindow* ExampleApp::window() { return state.window; }

GpuContext const& ExampleApp::gpu() { return state.gpu; }

usize ExampleApp::frame_count() { return state.frame_count; }

} // namespace wgpu::sandbox