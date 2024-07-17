#include "wgpu_utils.hpp"

#include <vector>

#ifdef __EMSCRIPTEN__
#include "emscripten/emscripten.h"
#else
#include "wgpu_glfw.h"
#endif

namespace wgpu
{
namespace
{

void report_features(Range<WGPUFeatureName const> const& features)
{
    for (auto const f : features)
        fmt::print("\t{} ({})\n", to_string(f), static_cast<int>(f));
}

void report_limits(WGPULimits const& limits)
{
    fmt::print("\tmaxTextureDimension1D: {}\n", limits.maxTextureDimension1D);
    fmt::print("\tmaxTextureDimension2D: {}\n", limits.maxTextureDimension2D);
    fmt::print("\tmaxTextureDimension3D: {}\n", limits.maxTextureDimension3D);
    fmt::print("\tmaxTextureArrayLayers: {}\n", limits.maxTextureArrayLayers);
    fmt::print("\tmaxBindGroups: {}\n", limits.maxBindGroups);
    fmt::print(
        "\tmaxDynamicUniformBuffersPerPipelineLayout: {}\n",
        limits.maxDynamicUniformBuffersPerPipelineLayout);
    fmt::print(
        "\tmaxDynamicStorageBuffersPerPipelineLayout: {}\n",
        limits.maxDynamicStorageBuffersPerPipelineLayout);
    fmt::print("\tmaxSampledTexturesPerShaderStage: {}\n", limits.maxSampledTexturesPerShaderStage);
    fmt::print("\tmaxSamplersPerShaderStage: {}\n", limits.maxSamplersPerShaderStage);
    fmt::print("\tmaxStorageBuffersPerShaderStage: {}\n", limits.maxStorageBuffersPerShaderStage);
    fmt::print("\tmaxStorageTexturesPerShaderStage: {}\n", limits.maxStorageTexturesPerShaderStage);
    fmt::print("\tmaxUniformBuffersPerShaderStage: {}\n", limits.maxUniformBuffersPerShaderStage);
    fmt::print("\tmaxUniformBufferBindingSize: {}\n", limits.maxUniformBufferBindingSize);
    fmt::print("\tmaxStorageBufferBindingSize: {}\n", limits.maxStorageBufferBindingSize);
    fmt::print("\tminUniformBufferOffsetAlignment: {}\n", limits.minUniformBufferOffsetAlignment);
    fmt::print("\tminStorageBufferOffsetAlignment: {}\n", limits.minStorageBufferOffsetAlignment);
    fmt::print("\tmaxVertexBuffers: {}\n", limits.maxVertexBuffers);
    fmt::print("\tmaxVertexAttributes: {}\n", limits.maxVertexAttributes);
    fmt::print("\tmaxVertexBufferArrayStride: {}\n", limits.maxVertexBufferArrayStride);
    fmt::print("\tmaxInterStageShaderComponents: {}\n", limits.maxInterStageShaderComponents);
    fmt::print("\tmaxComputeWorkgroupStorageSize: {}\n", limits.maxComputeWorkgroupStorageSize);
    fmt::print(
        "\tmaxComputeInvocationsPerWorkgroup: {}\n",
        limits.maxComputeInvocationsPerWorkgroup);
    fmt::print("\tmaxComputeWorkgroupSizeX: {}\n", limits.maxComputeWorkgroupSizeX);
    fmt::print("\tmaxComputeWorkgroupSizeY: {}\n", limits.maxComputeWorkgroupSizeY);
    fmt::print("\tmaxComputeWorkgroupSizeZ: {}\n", limits.maxComputeWorkgroupSizeZ);
    fmt::print("\tmaxComputeWorkgroupsPerDimension: {}\n", limits.maxComputeWorkgroupsPerDimension);
}

} // namespace

#ifdef __EMSCRIPTEN__
WGPUSurface make_surface(WGPUInstance const instance, char const* canvas_selector)
{
    WGPUSurfaceDescriptorFromCanvasHTMLSelector next_desc{};
    next_desc.chain.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
    next_desc.selector = canvas_selector;

    WGPUSurfaceDescriptor desc{};
    desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&next_desc);

    return wgpuInstanceCreateSurface(instance, &desc);
}
#else
WGPUSurface make_surface(WGPUInstance const instance, GLFWwindow* const window)
{
    return wgpu_make_surface_from_glfw(instance, window);
}
#endif

WGPUAdapter request_adapter(
    WGPUInstance const instance,
    WGPURequestAdapterOptions const* const options)
{
    struct Response
    {
        WGPUAdapter adapter;
        bool avail;
    };
    Response resp{};

    auto const callback = //
        [](WGPURequestAdapterStatus status,
           WGPUAdapter adapter,
           char const* message,
           void* userdata) {
            auto resp = static_cast<Response*>(userdata);
            if (status == WGPURequestAdapterStatus_Success)
                resp->adapter = adapter;
            else
                fmt::print("Could not get WebGPU adapter. Message: {}\n", message);

            resp->avail = true;
        };

    wgpuInstanceRequestAdapter(instance, options, callback, &resp);

#if __EMSCRIPTEN__
    // NOTE(dr): The call above is async when compiled with Emscripten so we wait on the result
    while (!resp.avail)
        emscripten_sleep(100);
#endif

    assert(resp.avail);
    return resp.adapter;
}

WGPUDevice request_device(WGPUAdapter const adapter, WGPUDeviceDescriptor const* const desc)
{
    struct Response
    {
        WGPUDevice device;
        bool avail;
    };
    Response resp{};

    auto const callback =
        [](WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* userdata) {
            auto resp = static_cast<Response*>(userdata);
            if (status == WGPURequestDeviceStatus_Success)
                resp->device = device;
            else
                fmt::print("Could not get WebGPU device. Message: {}\n", message);

            resp->avail = true;
        };

    wgpuAdapterRequestDevice(adapter, desc, callback, &resp);

#if __EMSCRIPTEN__
    // NOTE(dr): The call above is async when compiled with Emscripten so we wait on the result
    while (!resp.avail)
        emscripten_sleep(100);
#endif

    assert(resp.avail);
    return resp.device;
}

WGPUSurfaceTexture get_current_texture(WGPUSurface const surface)
{
    WGPUSurfaceTexture result;
    wgpuSurfaceGetCurrentTexture(surface, &result);
    return result;
}

WGPUTextureFormat get_preferred_texture_format(
    [[maybe_unused]] WGPUSurface const surface,
    [[maybe_unused]] WGPUAdapter const adapter)
{
#ifdef __EMSCRIPTEN__
    return wgpuSurfaceGetPreferredFormat(surface, adapter);
#else
    return WGPUTextureFormat_BGRA8Unorm;
#endif
}

void report_adapter_features(WGPUAdapter const adapter)
{
    std::size_t const num_features = wgpuAdapterEnumerateFeatures(adapter, nullptr);
    std::vector<WGPUFeatureName> features(num_features);
    wgpuAdapterEnumerateFeatures(adapter, features.data());

    fmt::print("Adapter features:\n");
    report_features({features.data(), num_features});
}

void report_adapter_limits(WGPUAdapter const adapter)
{
    WGPUSupportedLimits limits{};
    [[maybe_unused]] bool const ok = wgpuAdapterGetLimits(adapter, &limits);
    assert(ok);

    fmt::print("Adapter limits:\n");
    report_limits(limits.limits);
}

void report_adapter_properties(WGPUAdapter const adapter)
{
    WGPUAdapterProperties props{};
    wgpuAdapterGetProperties(adapter, &props);

    fmt::print("Adapter properties:\n");
    fmt::print("\tname: {}\n", props.name);
    fmt::print("\tvendorName: {}\n", props.vendorName);
    fmt::print("\tdeviceID: {}\n", props.deviceID);
    fmt::print("\tvendorID: {}\n", props.vendorID);
    if (props.driverDescription) fmt::print("\tdriverDescription: {}\n", props.driverDescription);
    fmt::print(
        "\tadapterType: {} ({})\n",
        to_string(props.adapterType),
        static_cast<int>(props.adapterType));
    fmt::print(
        "\tbackendType: {} ({})\n",
        to_string(props.backendType),
        static_cast<int>(props.backendType));
}

void report_device_features(WGPUDevice const device)
{
    std::size_t const num_features = wgpuDeviceEnumerateFeatures(device, nullptr);
    std::vector<WGPUFeatureName> features(num_features);
    wgpuDeviceEnumerateFeatures(device, features.data());

    fmt::print("Device features:\n");
    report_features({features.data(), num_features});
}

void report_device_limits(WGPUDevice const device)
{
    WGPUSupportedLimits limits{};
    [[maybe_unused]] bool const ok = wgpuDeviceGetLimits(device, &limits);
    assert(ok);

    fmt::print("Adapter limits:\n");
    report_limits(limits.limits);
}

void report_surface_capabilities(WGPUSurface const surface, WGPUAdapter const adapter)
{
    WGPUSurfaceCapabilities cap{};
    wgpuSurfaceGetCapabilities(surface, adapter, &cap);

    fmt::print("Surface capabilities:\n");

    fmt::print("\tformats:\n");
    for (std::size_t i = 0; i < cap.formatCount; ++i)
        fmt::print("\t\t{}\n", to_string(cap.formats[i]));

    fmt::print("\talphaModes:\n");
    for (std::size_t i = 0; i < cap.alphaModeCount; ++i)
        fmt::print("\t\t{}\n", to_string(cap.alphaModes[i]));

    fmt::print("\tpresentModes:\n");
    for (std::size_t i = 0; i < cap.presentModeCount; ++i)
        fmt::print("\t\t{}\n", to_string(cap.presentModes[i]));
}

char const* to_string(WGPUFeatureName const value)
{
    static constexpr char const* names[]{
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
    static constexpr char const* names[]{
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
    static constexpr char const* names[]{
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
    static constexpr char const* names[]{
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

char const* to_string(WGPUQueueWorkDoneStatus const value)
{
    static constexpr char const* names[]{
        "Success",
        "Error",
        "Unknown",
        "DeviceLost",
        "DeviceLost",
    };

    assert(value < std::size(names));
    return names[value];
}

char const* to_string(WGPUSurfaceGetCurrentTextureStatus const value)
{
    static constexpr char const* names[]{
        "Success",
        "Timeout",
        "Outdated",
        "Lost",
        "OutOfMemory",
        "DeviceLost",
    };

    assert(value < std::size(names));
    return names[value];
}

char const* to_string(WGPUTextureFormat const value)
{
    static constexpr char const* names[]{
        "Undefined",
        "R8Unorm",
        "R8Snorm",
        "R8Uint",
        "R8Sint",
        "R16Uint",
        "R16Sint",
        "R16Float",
        "RG8Unorm",
        "RG8Snorm",
        "RG8Uint",
        "RG8Sint",
        "R32Float",
        "R32Uint",
        "R32Sint",
        "RG16Uint",
        "RG16Sint",
        "RG16Float",
        "RGBA8Unorm",
        "RGBA8UnormSrgb",
        "RGBA8Snorm",
        "RGBA8Uint",
        "RGBA8Sint",
        "BGRA8Unorm",
        "BGRA8UnormSrgb",
        "RGB10A2Unorm",
        "RG11B10Ufloat",
        "RGB9E5Ufloat",
        "RG32Float",
        "RG32Uint",
        "RG32Sint",
        "RGBA16Uint",
        "RGBA16Sint",
        "RGBA16Float",
        "RGBA32Float",
        "RGBA32Uint",
        "RGBA32Sint",
        "Stencil8",
        "Depth16Unorm",
        "Depth24Plus",
        "Depth24PlusStencil8",
        "Depth32Float",
        "Depth32FloatStencil8",
        "BC1RGBAUnorm",
        "BC1RGBAUnormSrgb",
        "BC2RGBAUnorm",
        "BC2RGBAUnormSrgb",
        "BC3RGBAUnorm",
        "BC3RGBAUnormSrgb",
        "BC4RUnorm",
        "BC4RSnorm",
        "BC5RGUnorm",
        "BC5RGSnorm",
        "BC6HRGBUfloat",
        "BC6HRGBFloat",
        "BC7RGBAUnorm",
        "BC7RGBAUnormSrgb",
        "ETC2RGB8Unorm",
        "ETC2RGB8UnormSrgb",
        "ETC2RGB8A1Unorm",
        "ETC2RGB8A1UnormSrgb",
        "ETC2RGBA8Unorm",
        "ETC2RGBA8UnormSrgb",
        "EACR11Unorm",
        "EACR11Snorm",
        "EACRG11Unorm",
        "EACRG11Snorm",
        "ASTC4x4Unorm",
        "ASTC4x4UnormSrgb",
        "ASTC5x4Unorm",
        "ASTC5x4UnormSrgb",
        "ASTC5x5Unorm",
        "ASTC5x5UnormSrgb",
        "ASTC6x5Unorm",
        "ASTC6x5UnormSrgb",
        "ASTC6x6Unorm",
        "ASTC6x6UnormSrgb",
        "ASTC8x5Unorm",
        "ASTC8x5UnormSrgb",
        "ASTC8x6Unorm",
        "ASTC8x6UnormSrgb",
        "ASTC8x8Unorm",
        "ASTC8x8UnormSrgb",
        "ASTC10x5Unorm",
        "ASTC10x5UnormSrgb",
        "ASTC10x6Unorm",
        "ASTC10x6UnormSrgb",
        "ASTC10x8Unorm",
        "ASTC10x8UnormSrgb",
        "ASTC10x10Unorm",
        "ASTC10x10UnormSrgb",
        "ASTC12x10Unorm",
        "ASTC12x10UnormSrgb",
        "ASTC12x12Unorm",
        "ASTC12x12UnormSrgb",
    };

    assert(value < std::size(names));
    return names[value];
}

char const* to_string(WGPUCompositeAlphaMode const value)
{
    static constexpr char const* names[]{
        "Auto",
        "Opaque",
        "Premultiplied",
        "Unpremultiplied",
        "Inherit",
    };

    assert(value < std::size(names));
    return names[value];
}

char const* to_string(WGPUPresentMode const value)
{
    static constexpr char const* names[]{
        "Fifo",
        "FifoRelaxed",
        "Immediate",
        "Mailbox",
    };

    assert(value < std::size(names));
    return names[value];
}

char const* to_string(WGPUBufferMapAsyncStatus value)
{
    static constexpr char const* names[]{
        "Success",
        "ValidationError",
        "Unknown",
        "DeviceLost",
        "DestroyedBeforeCallback",
        "UnmappedBeforeCallback",
        "MappingAlreadyPending",
        "OffsetOutOfRange",
        "SizeOutOfRange",
    };

    assert(value < std::size(names));
    return names[value];
}

} // namespace wgpu
