#include <webgpu/glfw.h>

/*
    This is essentially a restructured version of https://github.com/eliemichel/glfw3webgpu
*/

#define WGPU_TARGET_MACOS 1
#define WGPU_TARGET_LINUX_X11 2
#define WGPU_TARGET_WINDOWS 3
#define WGPU_TARGET_LINUX_WAYLAND 4
#define WGPU_TARGET_EMSCRIPTEN 5

#if defined(__EMSCRIPTEN__)
#define WGPU_TARGET WGPU_TARGET_EMSCRIPTEN
#elif defined(_WIN32)
#define WGPU_TARGET WGPU_TARGET_WINDOWS
#elif defined(__APPLE__)
#define WGPU_TARGET WGPU_TARGET_MACOS
#elif defined(_GLFW_WAYLAND)
#define WGPU_TARGET WGPU_TARGET_LINUX_WAYLAND
#else
#define WGPU_TARGET WGPU_TARGET_LINUX_X11
#endif

#if WGPU_TARGET == WGPU_TARGET_MACOS
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>
#endif

#if WGPU_TARGET == WGPU_TARGET_MACOS
#define GLFW_EXPOSE_NATIVE_COCOA
#elif WGPU_TARGET == WGPU_TARGET_LINUX_X11
#define GLFW_EXPOSE_NATIVE_X11
#elif WGPU_TARGET == WGPU_TARGET_LINUX_WAYLAND
#define GLFW_EXPOSE_NATIVE_WAYLAND
#elif WGPU_TARGET == WGPU_TARGET_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#if !defined(__EMSCRIPTEN__)
#include <GLFW/glfw3native.h>
#endif

#if WGPU_TARGET == WGPU_TARGET_MACOS
typedef WGPUSurfaceDescriptorFromMetalLayer TargetSurfaceDesc;

static TargetSurfaceDesc targetSurfaceDesc(GLFWwindow* window)
{
    NSWindow* ns_window = glfwGetCocoaWindow(window);
    [ns_window.contentView setWantsLayer:YES];

    id metal_layer = [CAMetalLayer layer];
    [ns_window.contentView setLayer:metal_layer];

    // clang-format off
    return (TargetSurfaceDesc){
        .chain = {
            .next = NULL,
            .sType = WGPUSType_SurfaceDescriptorFromMetalLayer,
        },
        .layer = metal_layer,
    };
    // clang-format on
}

#elif WGPU_TARGET == WGPU_TARGET_LINUX_X11
typedef WGPUSurfaceDescriptorFromXlibWindow TargetSurfaceDesc;

static TargetSurfaceDesc targetSurfaceDesc(GLFWwindow* window)
{
    Display* x11_display = glfwGetX11Display();
    Window x11_window = glfwGetX11Window(window);

    // clang-format off
    return (TargetSurfaceDesc){
        .chain = {
            .next = NULL,
            .sType = WGPUSType_SurfaceDescriptorFromXlibWindow,
        },
        .display = x11_display,
        .window = x11_window,
    };
    // clang-format on
}

#elif WGPU_TARGET == WGPU_TARGET_LINUX_WAYLAND
typedef WGPUSurfaceDescriptorFromWaylandSurface TargetSurfaceDesc;

static TargetSurfaceDesc targetSurfaceDesc(GLFWwindow* window)
{
    struct wl_display* wayland_display = glfwGetWaylandDisplay();
    struct wl_surface* wayland_surface = glfwGetWaylandWindow(window);

    // clang-format off
    return (TargetSurfaceDesc){
        .chain = {
            .next = NULL,
            .sType = WGPUSType_SurfaceDescriptorFromWaylandSurface,
        },
        .display = wayland_display,
        .surface = wayland_surface,
    };
    // clang-format on
}

#elif WGPU_TARGET == WGPU_TARGET_WINDOWS
typedef WGPUSurfaceDescriptorFromWindowsHWND TargetSurfaceDesc;

static TargetSurfaceDesc targetSurfaceDesc(GLFWwindow* window)
{
    HWND hwnd = glfwGetWin32Window(window);
    HINSTANCE hinstance = GetModuleHandle(NULL);

    // clang-format off
    return (TargetSurfaceDesc){
        .chain = {
            .next = NULL,
            .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND,
        },
        .hinstance = hinstance,
        .hwnd = hwnd,
    };
    // clang-format on
}

#elif WGPU_TARGET == WGPU_TARGET_EMSCRIPTEN
typedef WGPUSurfaceDescriptorFromCanvasHTMLSelector TargetSurfaceDesc;

static TargetSurfaceDesc targetSurfaceDesc(GLFWwindow* /*window*/)
{
    // clang-format off
    return (TargetSurfaceDesc){
        .chain = {
            .next = NULL,
            .sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector,
        },
        .selector = "canvas",
    };
    // clang-format on
}

#else
#error "Unsupported WGPU_TARGET"

#endif

WGPUSurface glfwGetWGPUSurface(WGPUInstance instance, GLFWwindow* window)
{
    TargetSurfaceDesc const desc = targetSurfaceDesc(window);
    return wgpuInstanceCreateSurface(
        instance,
        &(WGPUSurfaceDescriptor){
            .label = NULL,
            .nextInChain = (WGPUChainedStruct const*)&desc,
        });
}
