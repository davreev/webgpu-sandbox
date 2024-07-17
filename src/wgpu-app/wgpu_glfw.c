#include "wgpu_glfw.h"

/*
    This is essentially a restructured version of https://github.com/eliemichel/glfw3webgpu
*/

#define WGPU_TARGET_LINUX_X11 1
#define WGPU_TARGET_LINUX_WAYLAND 2
#define WGPU_TARGET_MACOS 3
#define WGPU_TARGET_WINDOWS 4

#if defined(_WIN32)
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

#if WGPU_TARGET == WGPU_TARGET_LINUX_X11
#define GLFW_EXPOSE_NATIVE_X11
#elif WGPU_TARGET == WGPU_TARGET_LINUX_WAYLAND
#define GLFW_EXPOSE_NATIVE_WAYLAND
#elif WGPU_TARGET == WGPU_TARGET_MACOS
#define GLFW_EXPOSE_NATIVE_COCOA
#elif WGPU_TARGET == WGPU_TARGET_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <GLFW/glfw3native.h>

#if WGPU_TARGET == WGPU_TARGET_LINUX_X11
typedef WGPUSurfaceDescriptorFromXlibWindow TargetSurfaceDesc;

static TargetSurfaceDesc target_surface_desc(GLFWwindow* const window)
{
    // clang-format off
    return (TargetSurfaceDesc){
        .chain = {
            .next = NULL,
            .sType = WGPUSType_SurfaceDescriptorFromXlibWindow,
        },
        .display = glfwGetX11Display(),
        .window = glfwGetX11Window(window),
    };
    // clang-format on
}

#elif WGPU_TARGET == WGPU_TARGET_LINUX_WAYLAND
typedef WGPUSurfaceDescriptorFromWaylandSurface TargetSurfaceDesc;

static TargetSurfaceDesc target_surface_desc(GLFWwindow* const window)
{
    // clang-format off
    return (TargetSurfaceDesc){
        .chain = {
            .next = NULL,
            .sType = WGPUSType_SurfaceDescriptorFromWaylandSurface,
        },
        .display = glfwGetWaylandDisplay(),
        .surface = glfwGetWaylandWindow(window),
    };
    // clang-format on
}

#elif WGPU_TARGET == WGPU_TARGET_MACOS
typedef WGPUSurfaceDescriptorFromMetalLayer TargetSurfaceDesc;

static TargetSurfaceDesc target_surface_desc(GLFWwindow* const window)
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

#elif WGPU_TARGET == WGPU_TARGET_WINDOWS
typedef WGPUSurfaceDescriptorFromWindowsHWND TargetSurfaceDesc;

static TargetSurfaceDesc target_surface_desc(GLFWwindow* const window)
{
    // clang-format off
    return (TargetSurfaceDesc){
        .chain = {
            .next = NULL,
            .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND,
        },
        .hinstance = GetModuleHandle(NULL),
        .hwnd = glfwGetWin32Window(window),
    };
    // clang-format on
}

#else
#error "Unsupported WGPU_TARGET"

#endif

WGPUSurface wgpu_make_surface_from_glfw(WGPUInstance const instance, GLFWwindow* const window)
{
    TargetSurfaceDesc const desc = target_surface_desc(window);
    return wgpuInstanceCreateSurface(
        instance,
        &(WGPUSurfaceDescriptor){
            .label = NULL,
            .nextInChain = (WGPUChainedStruct const*)&desc,
        });
}
