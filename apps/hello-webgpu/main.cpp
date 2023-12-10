#include <cassert>

#include <fmt/core.h>

#include <webgpu/glfw.h>
#include <webgpu/webgpu.h>

int main(int /*argc*/, char** /*argv*/)
{
    WGPUInstanceDescriptor desc{};
    WGPUInstance const instance = wgpuCreateInstance(&desc);
    if (!instance)
    {
        fmt::print("Failed to create WebGPU instance\n");
        return 1;
    }

    if (!glfwInit())
    {
        fmt::print("Failed to initialize GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* const window = glfwCreateWindow(640, 480, "Hello WebGPU", nullptr, nullptr);
    if (!window)
    {
        fmt::print("Failed to create window\n");
        return 1;
    }
    WGPUSurface const surface = glfwGetWGPUSurface(instance, window);
    if(!surface)
    {
        fmt::print("Failed to get WebGPU surface\n");
        return 1;
    }

    while (!glfwWindowShouldClose(window))
        glfwPollEvents();

    wgpuSurfaceRelease(surface);
    wgpuInstanceRelease(instance);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
