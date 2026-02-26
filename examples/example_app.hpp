#pragma once

#include "gpu_context.hpp"

namespace wgpu::sandbox
{

struct ExampleApp
{
    struct Event
    {
        enum struct Type : u8
        {
            Undefined = 0,
            Key,
            MouseButton,
            Scroll,
            CursorMove,
            CursorEnter,
            WindowFocus,
            FramebufferResize,
        };

        Type type{};
        union
        {
            struct
            {
                i32 key{};
                i32 scancode{};
                i32 action{};
                i32 mods{};
            } key;
            struct
            {
                i32 button{};
                i32 action{};
                i32 mods{};
            } mouse_button;
            struct
            {
                f64 offset[2]{};
            } scroll;
            struct
            {
                f64 position[2]{};
            } cursor_move;
            struct
            {
                bool entered{};
            } cursor_enter;
            struct
            {
                bool focused{};
            } window_focus;
            struct
            {
                i32 width{};
                i32 height{};
            } framebuffer_resize;
        };
    };

    using Callback = void();
    using EventCallback = void(Event const& event);

    struct Desc
    {
        Callback* init_cb{};
        Callback* frame_cb{};
        Callback* deinit_cb{};
        EventCallback* event_cb{};
        struct
        {
            char const* title{};
            i32 width{};
            i32 height{};
        } window;
        char const* html_canvas_id{};
        void* userdata{};
    };

    static void run(Desc const& desc);

    static void ui_begin();

    static void ui_end();

    static void ui_draw(WGPURenderPassEncoder const encoder);

    static void* userdata();

    static GLFWwindow* window();

    static GpuContext const& gpu();

    static usize frame_count();
};

} // namespace wgpu::sandbox
