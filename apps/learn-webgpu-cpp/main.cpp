#include "GLFW/glfw3.h"
#include "webgpu/webgpu.h"
#include <cassert>
#include <vector>

#include <fmt/core.h>

#include <webgpu/glfw.h>

namespace
{

template <typename T>
struct Range
{
    T* ptr;
    std::size_t size;
    T* begin() const { return ptr; }
    T* end() const { return ptr + size; }
};

template <typename Func>
struct Deferred
{
    template <typename Func_>
    Deferred(Func_&& func) : func_(std::forward<Func_>(func))
    {
    }

    Deferred(Deferred const&) = delete;
    Deferred& operator=(Deferred const&) = delete;

    ~Deferred() { func_(); }

  private:
    Func func_;
};

template <typename Func>
[[nodiscard]] Deferred<Func> defer(Func&& func)
{
    return {std::forward<Func>(func)};
}

WGPUAdapter request_adapter(WGPUInstance const instance, WGPURequestAdapterOptions const* options)
{
    struct Result
    {
        WGPUAdapter adapter;
        bool is_ready;
    };
    Result result{};

    auto const callback = //
        [](WGPURequestAdapterStatus status,
           WGPUAdapter adapter,
           char const* message,
           void* userdata)
    {
        auto result = static_cast<Result*>(userdata);
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

WGPUDevice request_device(WGPUAdapter const adapter, WGPUDeviceDescriptor const* desc)
{
    struct Result
    {
        WGPUDevice device;
        bool is_ready;
    };
    Result result{};

    auto const callback =
        [](WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* userdata)
    {
        auto result = static_cast<Result*>(userdata);
        if (status == WGPURequestDeviceStatus_Success)
            result->device = device;
        else
            fmt::print("Could not get WebGPU device. Message: {}\n", message);

        result->is_ready = true;
    };

    wgpuAdapterRequestDevice(adapter, desc, callback, &result);
    assert(result.is_ready);

    return result.device;
}

char const* to_string(WGPUFeatureName const value)
{
    constexpr char const* names[]{
        "Undefined",
        "DepthClipControl",
        "Depth32FloatStencil8",
        "TimestampQuery",
        "TextureCompressionBC",
        "TextureCompressionETC2",
        "TextureCompressionASTC",
        "IndirectFirstInstance",
        "ShaderF16",
        "RG11B10UfloatRenderable",
        "BGRA8UnormStorage",
        "Float32Filterable",
    };

    if (value >= std::size(names))
        return "Unknown native feature";
    else
        return names[value];
}

char const* to_string(WGPUAdapterType const value)
{
    constexpr char const* names[]{
        "DiscreteGPU",
        "IntegratedGPU",
        "CPU",
        "Unknown",
    };

    assert(value < std::size(names));
    return names[value];
}

char const* to_string(WGPUBackendType const value)
{
    constexpr char const* names[]{
        "Undefined",
        "Null",
        "WebGPU",
        "D3D11",
        "D3D12",
        "Metal",
        "Vulkan",
        "OpenGL",
        "OpenGLES",
    };

    assert(value < std::size(names));
    return names[value];
}

char const* to_string(WGPUErrorType const value)
{
    constexpr char const* names[]{
        "None",
        "Validation",
        "OutOfMemory",
        "Internal",
        "Unknown",
        "DeviceLost",
    };

    assert(value < std::size(names));
    return names[value];
}

void report_features(Range<WGPUFeatureName const> const& features)
{
    for (auto const f : features)
        fmt::print("\t- {} ({})\n", to_string(f), static_cast<int>(f));
}

void report_features(WGPUAdapter const adapter)
{
    std::size_t const num_features = wgpuAdapterEnumerateFeatures(adapter, nullptr);
    std::vector<WGPUFeatureName> features(num_features);
    wgpuAdapterEnumerateFeatures(adapter, features.data());

    fmt::print("Adapter features:\n");
    report_features({features.data(), num_features});
}

void report_features(WGPUDevice const device)
{
    std::size_t const num_features = wgpuDeviceEnumerateFeatures(device, nullptr);
    std::vector<WGPUFeatureName> features(num_features);
    wgpuDeviceEnumerateFeatures(device, features.data());

    fmt::print("Device features:\n");
    report_features({features.data(), num_features});
}

void report_limits(WGPULimits const& limits)
{
    fmt::print("\t- maxTextureDimension1D: {}\n", limits.maxTextureDimension1D);
    fmt::print("\t- maxTextureDimension2D: {}\n", limits.maxTextureDimension2D);
    fmt::print("\t- maxTextureDimension3D: {}\n", limits.maxTextureDimension3D);
    fmt::print("\t- maxTextureArrayLayers: {}\n", limits.maxTextureArrayLayers);
    fmt::print("\t- maxBindGroups: {}\n", limits.maxBindGroups);
    fmt::print(
        "\t- maxDynamicUniformBuffersPerPipelineLayout: {}\n",
        limits.maxDynamicUniformBuffersPerPipelineLayout);
    fmt::print(
        "\t- maxDynamicStorageBuffersPerPipelineLayout: {}\n",
        limits.maxDynamicStorageBuffersPerPipelineLayout);
    fmt::print(
        "\t- maxSampledTexturesPerShaderStage: {}\n",
        limits.maxSampledTexturesPerShaderStage);
    fmt::print("\t- maxSamplersPerShaderStage: {}\n", limits.maxSamplersPerShaderStage);
    fmt::print("\t- maxStorageBuffersPerShaderStage: {}\n", limits.maxStorageBuffersPerShaderStage);
    fmt::print(
        "\t- maxStorageTexturesPerShaderStage: {}\n",
        limits.maxStorageTexturesPerShaderStage);
    fmt::print("\t- maxUniformBuffersPerShaderStage: {}\n", limits.maxUniformBuffersPerShaderStage);
    fmt::print("\t- maxUniformBufferBindingSize: {}\n", limits.maxUniformBufferBindingSize);
    fmt::print("\t- maxStorageBufferBindingSize: {}\n", limits.maxStorageBufferBindingSize);
    fmt::print("\t- minUniformBufferOffsetAlignment: {}\n", limits.minUniformBufferOffsetAlignment);
    fmt::print("\t- minStorageBufferOffsetAlignment: {}\n", limits.minStorageBufferOffsetAlignment);
    fmt::print("\t- maxVertexBuffers: {}\n", limits.maxVertexBuffers);
    fmt::print("\t- maxVertexAttributes: {}\n", limits.maxVertexAttributes);
    fmt::print("\t- maxVertexBufferArrayStride: {}\n", limits.maxVertexBufferArrayStride);
    fmt::print("\t- maxInterStageShaderComponents: {}\n", limits.maxInterStageShaderComponents);
    fmt::print("\t- maxComputeWorkgroupStorageSize: {}\n", limits.maxComputeWorkgroupStorageSize);
    fmt::print(
        "\t- maxComputeInvocationsPerWorkgroup: {}\n",
        limits.maxComputeInvocationsPerWorkgroup);
    fmt::print("\t- maxComputeWorkgroupSizeX: {}\n", limits.maxComputeWorkgroupSizeX);
    fmt::print("\t- maxComputeWorkgroupSizeY: {}\n", limits.maxComputeWorkgroupSizeY);
    fmt::print("\t- maxComputeWorkgroupSizeZ: {}\n", limits.maxComputeWorkgroupSizeZ);
    fmt::print(
        "\t- maxComputeWorkgroupsPerDimension: {}\n",
        limits.maxComputeWorkgroupsPerDimension);
}

void report_limits(WGPUAdapter const adapter)
{
    WGPUSupportedLimits limits{};
    bool const ok = wgpuAdapterGetLimits(adapter, &limits);
    assert(ok);

    fmt::print("Adapter limits:\n");
    report_limits(limits.limits);
}

void report_limits(WGPUDevice const device)
{
    WGPUSupportedLimits limits{};
    bool const ok = wgpuDeviceGetLimits(device, &limits);
    assert(ok);

    fmt::print("Adapter limits:\n");
    report_limits(limits.limits);
}

void report_properties(WGPUAdapter const adapter)
{
    WGPUAdapterProperties props{};
    wgpuAdapterGetProperties(adapter, &props);

    fmt::print("Adapter properties;\n");
    fmt::print("\t- name: {}\n", props.name);
    fmt::print("\t- vendorName: {}\n", props.vendorName);
    fmt::print("\t- deviceID: {}\n", props.deviceID);
    fmt::print("\t- vendorID: {}\n", props.vendorID);
    if (props.driverDescription)
        fmt::print("\t- driverDescription: {}\n", props.driverDescription);
    fmt::print(
        "\t- adapterType: {} ({})\n",
        to_string(props.adapterType),
        static_cast<int>(props.adapterType));
    fmt::print(
        "\t- backendType: {} ({})\n",
        to_string(props.backendType),
        static_cast<int>(props.backendType));
}

void device_error_cb(WGPUErrorType type, char const* message, void* /*userdata*/)
{
    fmt::print(
        "Device error: {} ({})\nMessage: {}\n",
        to_string(type),
        static_cast<int>(type),
        message);
}

} // namespace

int main(int /*argc*/, char** /*argv*/)
{
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
    auto const rel_instance = defer([=]() { wgpuInstanceRelease(instance); });

    // Initialize GLFW
    if (!glfwInit())
    {
        fmt::print("Failed to initialize GLFW\n");
        return 1;
    }
    auto const rel_glfw = defer([]() { glfwTerminate(); });

    // Create window via GLFW and get WebGPU surface
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* const window = glfwCreateWindow(640, 480, "WebGPU C++ Guide", nullptr, nullptr);
    if (!window)
    {
        fmt::print("Failed to create window\n");
        return 1;
    }
    auto const rel_window = defer([=]() { glfwDestroyWindow(window); });

    WGPUSurface const surface = glfwGetWGPUSurface(instance, window);
    if (!surface)
    {
        fmt::print("Failed to get WebGPU surface\n");
        return 1;
    }
    auto const rel_surface = defer([=]() { wgpuSurfaceRelease(surface); });

    // Create WebGPU adapter
    WGPUAdapter adapter{};
    {
        WGPURequestAdapterOptions options{};
        options.compatibleSurface = surface;
        adapter = request_adapter(instance, &options);
        if (!adapter)
        {
            fmt::print("Failed to get WebGPU adapter\n");
            return 1;
        }

        report_features(adapter);
        report_limits(adapter);
        report_properties(adapter);
    }
    auto const rel_adapter = defer([=]() { wgpuAdapterRelease(adapter); });

    // Create WebGPU device
    WGPUDevice device{};
    {
        WGPUDeviceDescriptor desc{};
        desc.label = "My device";
        // desc.requiredFeatureCount = 0;
        // desc.requiredLimits = nullptr;
        // desc.defaultQueue.nextInChain = nullptr;
        desc.defaultQueue.label = "The default queue";

        device = request_device(adapter, &desc);
        if (!device)
        {
            fmt::print("Failed to get WebGPU device\n");
            return 1;
        }

        wgpuDeviceSetUncapturedErrorCallback(device, device_error_cb, nullptr);

        report_features(device);
        report_limits(device);
    }
    auto const rel_device = defer([=]() { wgpuDeviceRelease(device); });

    // Start frame loop
    while (!glfwWindowShouldClose(window))
        glfwPollEvents();

    // NOTE(dr): Cleanup happens in correct order automatically via deferred lambdas

    return 0;
}
