#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu_cpp.h>

#include <GLFW/glfw3.h>

namespace
{

wgpu::Adapter requestAdapter(
    wgpu::Instance const& instance,
    wgpu::RequestAdapterOptions const* options)
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

    instance.RequestAdapter(options, callback, &result);
    assert(result.is_ready);

    return result.adapter;
}

} // namespace

int main(int /*argc*/, char** /*argv*/)
{
    wgpu::InstanceDescriptor desc{};
    desc.nextInChain = nullptr;

    wgpu::Instance instance = wgpu::CreateInstance(&desc);

    if (!instance)
    {
        fmt::print("WebGPU initialization failed\n");
        return 1;
    }
    else
    {
        fmt::print("WebGPU initialization succeeded\n");
    }

    // fmt::print("WGPU instance: {}", instance);
    // wgpu::InstanceRelease(instance);

    return 0;
}
