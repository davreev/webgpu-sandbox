#include "wgpu_utils.hpp"

#include <cassert>

#include <fmt/core.h>

#ifdef __EMSCRIPTEN__
#include "emsc_utils.hpp"
#else
#include "wgpu_glfw.h"
#endif

namespace wgpu::sandbox
{
namespace
{

void report_features(WGPUSupportedFeatures const& features)
{
    for (std::size_t i = 0; i < features.featureCount; ++i)
    {
        WGPUFeatureName const name = features.features[i];
        fmt::println("\t{} ({})", to_string(name), int(name));
    }
}

void report_limits(WGPULimits const& limits)
{
    fmt::println("\tmaxTextureDimension1D: {}", limits.maxTextureDimension1D);
    fmt::println("\tmaxTextureDimension2D: {}", limits.maxTextureDimension2D);
    fmt::println("\tmaxTextureDimension3D: {}", limits.maxTextureDimension3D);
    fmt::println("\tmaxTextureArrayLayers: {}", limits.maxTextureArrayLayers);
    fmt::println("\tmaxBindGroups: {}", limits.maxBindGroups);
    fmt::println("\tmaxBindGroupsPlusVertexBuffers: {}", limits.maxBindGroupsPlusVertexBuffers);
    fmt::println("\tmaxBindingsPerBindGroup: {}", limits.maxBindingsPerBindGroup);
    fmt::println(
        "\tmaxDynamicUniformBuffersPerPipelineLayout: {}",
        limits.maxDynamicUniformBuffersPerPipelineLayout);
    fmt::println(
        "\tmaxDynamicStorageBuffersPerPipelineLayout: {}",
        limits.maxDynamicStorageBuffersPerPipelineLayout);
    fmt::println("\tmaxSampledTexturesPerShaderStage: {}", limits.maxSampledTexturesPerShaderStage);
    fmt::println("\tmaxSamplersPerShaderStage: {}", limits.maxSamplersPerShaderStage);
    fmt::println("\tmaxStorageBuffersPerShaderStage: {}", limits.maxStorageBuffersPerShaderStage);
    fmt::println("\tmaxStorageTexturesPerShaderStage: {}", limits.maxStorageTexturesPerShaderStage);
    fmt::println("\tmaxUniformBuffersPerShaderStage: {}", limits.maxUniformBuffersPerShaderStage);
    fmt::println("\tmaxUniformBufferBindingSize: {}", limits.maxUniformBufferBindingSize);
    fmt::println("\tmaxStorageBufferBindingSize: {}", limits.maxStorageBufferBindingSize);
    fmt::println("\tminUniformBufferOffsetAlignment: {}", limits.minUniformBufferOffsetAlignment);
    fmt::println("\tminStorageBufferOffsetAlignment: {}", limits.minStorageBufferOffsetAlignment);
    fmt::println("\tmaxVertexBuffers: {}", limits.maxVertexBuffers);
    fmt::println("\tmaxBufferSize: {}", limits.maxBufferSize);
    fmt::println("\tmaxVertexAttributes: {}", limits.maxVertexAttributes);
    fmt::println("\tmaxVertexBufferArrayStride: {}", limits.maxVertexBufferArrayStride);
    fmt::println("\tmaxInterStageShaderComponents: {}", limits.maxInterStageShaderVariables);
    fmt::println("\tmaxColorAttachments: {}", limits.maxColorAttachments);
    fmt::println("\tmaxColorAttachmentBytesPerSample: {}", limits.maxColorAttachmentBytesPerSample);
    fmt::println("\tmaxComputeWorkgroupStorageSize: {}", limits.maxComputeWorkgroupStorageSize);
    fmt::println(
        "\tmaxComputeInvocationsPerWorkgroup: {}",
        limits.maxComputeInvocationsPerWorkgroup);
    fmt::println("\tmaxComputeWorkgroupSizeX: {}", limits.maxComputeWorkgroupSizeX);
    fmt::println("\tmaxComputeWorkgroupSizeY: {}", limits.maxComputeWorkgroupSizeY);
    fmt::println("\tmaxComputeWorkgroupSizeZ: {}", limits.maxComputeWorkgroupSizeZ);
    fmt::println("\tmaxComputeWorkgroupsPerDimension: {}", limits.maxComputeWorkgroupsPerDimension);
}

} // namespace

WGPUSurface make_surface(WGPUInstance const instance, SurfaceSource const& surface_src)
{
#ifdef __EMSCRIPTEN__
    WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvas_desc{};
    canvas_desc.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
    canvas_desc.selector = {surface_src.canvas_id, WGPU_STRLEN};

    WGPUSurfaceDescriptor desc{};
    desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&canvas_desc);

    return wgpuInstanceCreateSurface(instance, &desc);
#else
    return wgpu_make_surface_from_glfw(instance, surface_src.window);
#endif
}

WGPUWaitStatus wait_for_future(
    WGPUInstance const instance,
    WGPUFuture const future,
    std::uint64_t const timeout)
{
    WGPUFutureWaitInfo info = {};
    info.future = future;

    // NOTE(dr): This function isn't implemented by wgpu-native yet and will panic at runtime
    // (https://github.com/gfx-rs/wgpu-native/issues/510)
    return wgpuInstanceWaitAny(instance, 1, &info, timeout);
}

WGPUAdapter request_adapter(
    WGPUInstance const instance,
    WGPURequestAdapterOptions const* const options)
{
    struct ReqResult
    {
        WGPUAdapter adapter;
        bool is_ready;
    } result{};

    WGPURequestAdapterCallbackInfo cb_info{};
    cb_info.userdata1 = &result;
    cb_info.mode = WGPUCallbackMode_AllowSpontaneous;
    cb_info.callback = //
        [](WGPURequestAdapterStatus status,
           WGPUAdapter adapter,
           WGPUStringView message,
           void* userdata1,
           void* /*userdata2*/) {
            auto result = static_cast<ReqResult*>(userdata1);
            if (status == WGPURequestAdapterStatus_Success)
                result->adapter = adapter;
            else
                fmt::println("Could not get WebGPU adapter. Message: {}", message.data);
#ifdef __EMSCRIPTEN__
            raise_event("wgpuAdapterReady");
#else
            result->is_ready = true;
#endif
        };

    [[maybe_unused]]
    WGPUFuture const fut = wgpuInstanceRequestAdapter(instance, options, cb_info);

    // Wait until async operation is done
#ifdef __EMSCRIPTEN__
    wait_for_event("wgpuAdapterReady");
#else
    // NOTE(dr): Waiting on futures is not yet implemented in wgpu-native
    // wait_for_future(instance, fut);
    wait_for_condition(instance, [&]() { return result.is_ready; });
#endif

    assert(result.adapter);
    return result.adapter;
}

WGPUDevice request_device(
    [[maybe_unused]] WGPUInstance const instance,
    WGPUAdapter const adapter,
    WGPUDeviceDescriptor const* const desc)
{
    struct ReqResult
    {
        WGPUDevice device;
        bool is_ready;
    } result{};

    WGPURequestDeviceCallbackInfo cb_info{};
    cb_info.userdata1 = &result;
    cb_info.mode = WGPUCallbackMode_AllowSpontaneous;
    cb_info.callback = //
        [](WGPURequestDeviceStatus status,
           WGPUDevice device,
           WGPUStringView message,
           void* userdata1,
           void* /*userdata2*/) {
            auto result = static_cast<ReqResult*>(userdata1);
            if (status == WGPURequestDeviceStatus_Success)
                result->device = device;
            else
                fmt::println("Could not get WebGPU device. Message: {}", message.data);
#ifdef __EMSCRIPTEN__
            raise_event("wgpuDeviceReady");
#else
            result->is_ready = true;
#endif
        };

    [[maybe_unused]]
    WGPUFuture const fut = wgpuAdapterRequestDevice(adapter, desc, cb_info);

    // Wait until async operation is done
#ifdef __EMSCRIPTEN__
    wait_for_event("wgpuDeviceReady");
#else
    // NOTE(dr): Waiting on futures is not yet implemented in wgpu-native
    // wait_for_future(instance, fut);
    wait_for_condition(instance, [&]() { return result.is_ready; });
#endif

    assert(result.device);
    return result.device;
}

void report_adapter_features(WGPUAdapter const adapter)
{
    WGPUSupportedFeatures features;
    wgpuAdapterGetFeatures(adapter, &features);

    fmt::println("Adapter features:");
    report_features(features);
}

void report_adapter_limits(WGPUAdapter const adapter)
{
    WGPULimits limits{};
    [[maybe_unused]]
    auto const status = wgpuAdapterGetLimits(adapter, &limits);
    assert(status == WGPUStatus_Success);

    fmt::println("Adapter limits:");
    report_limits(limits);
}

void report_adapter_properties(WGPUAdapter const adapter)
{
    WGPUAdapterInfo info{};
    wgpuAdapterGetInfo(adapter, &info);

    fmt::println("Adapter properties:");
    fmt::println("\tvendor: {} (id: {})", info.vendor.data, info.vendorID);
    fmt::println("\tdevice: {} (id: {})", info.device.data, info.deviceID);
    if (info.architecture.data)
        fmt::println("\tarchitecture: {}", info.architecture.data);
    fmt::println("\tdescription: {}", info.description.data);
    fmt::println("\tadapterType: {} ({})", to_string(info.adapterType), int(info.adapterType));
    fmt::println("\tbackendType: {} ({})", to_string(info.backendType), int(info.backendType));
}

void report_device_features(WGPUDevice const device)
{
    WGPUSupportedFeatures features;
    wgpuDeviceGetFeatures(device, &features);
    fmt::println("Device features:");
    report_features(features);
}

void report_device_limits(WGPUDevice const device)
{
    WGPULimits limits{};
    [[maybe_unused]]
    auto const status = wgpuDeviceGetLimits(device, &limits);
    assert(status == WGPUStatus::WGPUStatus_Success);

    fmt::println("Device limits:");
    report_limits(limits);
}

void report_surface_capabilities(WGPUSurface const surface, WGPUAdapter const adapter)
{
    WGPUSurfaceCapabilities cap{};
    wgpuSurfaceGetCapabilities(surface, adapter, &cap);

    fmt::println("Surface capabilities:");
    fmt::println("\tformats:");
    for (std::size_t i = 0; i < cap.formatCount; ++i)
        fmt::println("\t\t{}", to_string(cap.formats[i]));

    fmt::println("\talphaModes:");
    for (std::size_t i = 0; i < cap.alphaModeCount; ++i)
        fmt::println("\t\t{}", to_string(cap.alphaModes[i]));

    fmt::println("\tpresentModes:");
    for (std::size_t i = 0; i < cap.presentModeCount; ++i)
        fmt::println("\t\t{}", to_string(cap.presentModes[i]));
}

char const* to_string(WGPUFeatureName const value)
{
    static constexpr char const* names[]{
        "Undefined",
        "DepthClipControl",
        "Depth32FloatStencil8",
        "TimestampQuery",
        "TextureCompressionBC",
        "TextureCompressionBCSliced3D",
        "TextureCompressionETC2",
        "TextureCompressionASTC",
        "TextureCompressionASTCSliced3D",
        "IndirectFirstInstance",
        "ShaderF16",
        "RG11B10UfloatRenderable",
        "BGRA8UnormStorage",
        "Float32Filterable",
        "Float32Blendable",
        "ClipDistances",
        "DualSourceBlending",
    };
    if (value >= std::size(names))
        return "Unknown native feature";
    else
        return names[value];
}

char const* to_string(WGPUAdapterType const value)
{
    static constexpr char const* names[]{
        "Undefined",
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
        "Undefined",
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
        "Undefined",
        "Success",
        "InstanceDropped",
        "Error",
        "Unknown",
    };
    assert(value < std::size(names));
    return names[value];
}

char const* to_string(WGPUSurfaceGetCurrentTextureStatus const value)
{
    static constexpr char const* names[]{
        "Undefined",
        "SuccessOptimal",
        "SuccessSuboptimal",
        "Timeout",
        "Outdated",
        "Lost",
        "OutOfMemory",
        "DeviceLost",
        "Error",
    };
    assert(value < std::size(names));
    return names[value];
}

char const* to_string(WGPUTextureFormat const value)
{
    // clang-format off
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
        "RGB10A2Uint",
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
    // clang-format on
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
        "Undefined",
        "Fifo",
        "FifoRelaxed",
        "Immediate",
        "Mailbox",
    };
    assert(value < std::size(names));
    return names[value];
}

char const* to_string(WGPUMapAsyncStatus value)
{
    static constexpr char const* names[]{
        "Undefined",
        "Success",
        "InstanceDropped",
        "Error",
        "Aborted",
        "Unknown",
    };
    assert(value < std::size(names));
    return names[value];
}

} // namespace wgpu::sandbox
