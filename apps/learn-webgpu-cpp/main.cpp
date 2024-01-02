#include <cassert>
#include <vector>

#include <fmt/core.h>

#include <webgpu/glfw.h>
#include <webgpu/webgpu.h>

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

char const* to_string(WGPUQueueWorkDoneStatus const value)
{
    constexpr char const* names[]{
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
    constexpr char const* names[]{
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
    constexpr char const* names[]{
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
    constexpr char const* names[]{
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
    constexpr char const* names[]{
        "Fifo",
        "FifoRelaxed",
        "Immediate",
        "Mailbox",
    };

    assert(value < std::size(names));
    return names[value];
}

void report_features(Range<WGPUFeatureName const> const& features)
{
    for (auto const f : features)
        fmt::print("\t{} ({})\n", to_string(f), static_cast<int>(f));
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
    fmt::print(
        "\tmaxSampledTexturesPerShaderStage: {}\n",
        limits.maxSampledTexturesPerShaderStage);
    fmt::print("\tmaxSamplersPerShaderStage: {}\n", limits.maxSamplersPerShaderStage);
    fmt::print("\tmaxStorageBuffersPerShaderStage: {}\n", limits.maxStorageBuffersPerShaderStage);
    fmt::print(
        "\tmaxStorageTexturesPerShaderStage: {}\n",
        limits.maxStorageTexturesPerShaderStage);
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
    fmt::print(
        "\tmaxComputeWorkgroupsPerDimension: {}\n",
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

    fmt::print("Adapter properties:\n");
    fmt::print("\tname: {}\n", props.name);
    fmt::print("\tvendorName: {}\n", props.vendorName);
    fmt::print("\tdeviceID: {}\n", props.deviceID);
    fmt::print("\tvendorID: {}\n", props.vendorID);
    if (props.driverDescription)
        fmt::print("\tdriverDescription: {}\n", props.driverDescription);
    fmt::print(
        "\tadapterType: {} ({})\n",
        to_string(props.adapterType),
        static_cast<int>(props.adapterType));
    fmt::print(
        "\tbackendType: {} ({})\n",
        to_string(props.backendType),
        static_cast<int>(props.backendType));
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
    constexpr int window_width{640};
    constexpr int window_height{480};

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

    // Create GLFW window
    GLFWwindow* window{};
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow( //
            window_width,
            window_height,
            "Learn WebGPU C++",
            nullptr,
            nullptr);

        if (!window)
        {
            fmt::print("Failed to create window\n");
            return 1;
        }
    }
    auto const rel_window = defer([=]() { glfwDestroyWindow(window); });

    // Get WebGPU surface from GLFW window
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
        report_surface_capabilities(surface, adapter);
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

    // Get the device's queue
    WGPUQueue const queue = wgpuDeviceGetQueue(device);
    {
        // Add callback which fires whenever queued work is done
        auto const work_done_cb = [](WGPUQueueWorkDoneStatus const status, void* /*userdata*/)
        { fmt::print("Queued work finished with status: {}\n", to_string(status)); };

        wgpuQueueOnSubmittedWorkDone(queue, work_done_cb, nullptr);
    }

    // Configure surface via the device (replaces swap chain API)
    WGPUSurfaceConfiguration config{};
    {
        config.device = device;
        config.width = window_width;
        config.height = window_height;
        config.format = wgpuSurfaceGetPreferredFormat(surface, adapter);
        config.usage = WGPUTextureUsage_RenderAttachment;
        config.presentMode = WGPUPresentMode_Mailbox;
    }
    wgpuSurfaceConfigure(surface, &config);
    auto const unconfig_srf = defer([&] { wgpuSurfaceUnconfigure(surface); });

    // Start frame loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Get the texture to render the frame to
        WGPUSurfaceTexture srf_tex;
        wgpuSurfaceGetCurrentTexture(surface, &srf_tex);
        if (srf_tex.status != WGPUSurfaceGetCurrentTextureStatus_Success)
        {
            fmt::print("Failed to get surface texture ({})\n", to_string(srf_tex.status));
            continue;
        }

        // Create a command encoder from the device
        WGPUCommandEncoder encoder{};
        {
            WGPUCommandEncoderDescriptor desc{};
            desc.label = "My encoder";
            encoder = wgpuDeviceCreateCommandEncoder(device, &desc);
        }
        auto const rel_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // Can use debug markers on the encoder for printf-like debugging bw commands
        // wgpuCommandEncoderInsertDebugMarker(encoder, "Do a thing");
        // wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

        // Create render pass via the encoder
        WGPURenderPassEncoder pass{};
        {
            WGPUTextureView view{};
            {
                WGPUTextureViewDescriptor desc{};
                {
                    desc.mipLevelCount = 1;
                    desc.arrayLayerCount = 1;
                }
                view = wgpuTextureCreateView(srf_tex.texture, &desc);
                assert(view);
            }
            // NOTE(dr): TextureView doesn't need to be released!
            // auto const rel_view = defer([&]() { wgpuTextureViewRelease(view); });

            WGPURenderPassColorAttachment color{};
            {
                color.view = view;
                color.loadOp = WGPULoadOp_Clear;
                color.storeOp = WGPUStoreOp_Store;
                color.clearValue = WGPUColor{1.0, 0.3, 0.3, 1.0};
            }

            WGPURenderPassDescriptor desc{};
            {
                desc.colorAttachments = &color;
                desc.colorAttachmentCount = 1;
                // ...
            }

            pass = wgpuCommandEncoderBeginRenderPass(encoder, &desc);
        }

        // NOTE(dr): Render pass clears the screen by default
        // ...
        // ...
        // ...

        wgpuRenderPassEncoderEnd(pass);

        // Create a command via the encoder
        WGPUCommandBuffer command{};
        {
            WGPUCommandBufferDescriptor desc{};
            desc.label = "My command buffer";
            command = wgpuCommandEncoderFinish(encoder, &desc);
        }

        // Submit encoded command
        wgpuQueueSubmit(queue, 1, &command);

        // Display the result
        wgpuSurfacePresent(surface);
    }

    return 0;
}
