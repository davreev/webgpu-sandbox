#include <cassert>

#include <fmt/core.h>

#include <webgpu/glfw.h>

namespace
{

WGPUAdapter requestAdapter(WGPUInstance const& instance, WGPURequestAdapterOptions const* options)
{
    struct Result
    {
        WGPUAdapter adapter{nullptr};
        bool is_ready{false};
    };
    Result result;

    auto const callback = //
        [](WGPURequestAdapterStatus status,
           WGPUAdapter adapter,
           char const* message,
           void* user_data)
    {
        auto result = static_cast<Result*>(user_data);
        if (status == WGPURequestAdapterStatus_Success)
            result->adapter = adapter;
        else
            fmt::print("Could not get WebGPU adapter. Message: {}\n", message);

        result->is_ready = true;
    };

    wgpuInstanceRequestAdapter(instance, options, callback, &result);
    assert(result.is_ready);

    return result.adapter;
}

} // namespace

int main(int /*argc*/, char** /*argv*/)
{
    // TODO(dr): https://github.com/eliemichel/glfw3webgpu/blob/main/README.md#example

    WGPUInstanceDescriptor desc{};
    desc.nextInChain = nullptr;

    WGPUInstance instance = wgpuCreateInstance(&desc);

    if (!instance)
    {
        fmt::print("WebGPU initialization failed\n");
        return 1;
    }
    else
    {
        fmt::print("WebGPU initialization succeeded\n");
    }

    if (!glfwInit())
    {
        fmt::print("Failed to initialize GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* const window = glfwCreateWindow(640, 480, "Hello Triangle", nullptr, nullptr);
    if (!window)
    {
        fmt::print("Failed to create window\n");
        return 1;
    }

    // TODO(dr)
    WGPUSurface surface = glfwGetWGPUSurface(instance, window);






    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    // fmt::print("WGPU instance: {}", instance);
    // wgpu::InstanceRelease(instance);

    return 0;
}
