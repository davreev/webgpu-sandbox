#include <cassert>

#include <fmt/core.h>

#include <webgpu/app.hpp>
#include <webgpu/glfw.h>
#include <webgpu/webgpu.h>

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu;

    // Create WebGPU instance
    WGPUInstance instance{};
    {
        WGPUInstanceDescriptor desc{};
        instance = wgpuCreateInstance(&desc);
        if (!instance)
        {
            fmt::print("Failed to create WebGPU instance\n");
            return 1;
        }
    }
    auto const drop_instance = defer([=]() { wgpuInstanceRelease(instance); });

    // Initialize GLFW
    if (!glfwInit())
    {
        fmt::print("Failed to initialize GLFW\n");
        return 1;
    }
    auto const drop_glfw = defer([]() { glfwTerminate(); });

    // Create GLFW window
    GLFWwindow* window{};
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(640, 480, "Hello WebGPU", nullptr, nullptr);
        if (!window)
        {
            fmt::print("Failed to create window\n");
            return 1;
        }
    }
    auto const drop_window = defer([=]() { glfwDestroyWindow(window); });

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
