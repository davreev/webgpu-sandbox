#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include <webgpu/app.hpp>
#include <webgpu/glfw.h>

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu;

    // Initialize GLFW
    if (!glfwInit())
    {
        fmt::print("Failed to initialize GLFW\n");
        return 1;
    }
    auto const deinit_glfw = defer([]() { glfwTerminate(); });

    // Create GLFW window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* const window = glfwCreateWindow(640, 480, "Hello WebGPU", nullptr, nullptr);
    if (!window)
    {
        fmt::print("Failed to create window\n");
        return 1;
    }
    auto const drop_window = defer([=]() { glfwDestroyWindow(window); });

    // Create WebGPU instance
    WGPUInstance const instance = wgpuCreateInstance(nullptr);
    if (!instance)
    {
        fmt::print("Failed to create WebGPU instance\n");
        return 1;
    }
    auto const drop_instance = defer([=]() { wgpuInstanceRelease(instance); });

    // Get WebGPU surface from GLFW window
    WGPUSurface const surface = glfwGetWGPUSurface(instance, window);
    if (!surface)
    {
        fmt::print("Failed to get WebGPU surface\n");
        return 1;
    }
    auto const drop_surface = defer([=]() { wgpuSurfaceRelease(surface); });

    // Frame loop
    while (!glfwWindowShouldClose(window))
        glfwPollEvents();

    return 0;
}
