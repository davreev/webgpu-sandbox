#include "gpu_context.hpp"

#include <cassert>

#include <fmt/core.h>

namespace wgpu::sandbox
{
namespace
{

template <typename T>
T make_default()
{
    return {};
}

template <>
WGPUDeviceDescriptor make_default()
{
    WGPUDeviceDescriptor result = {};
    result.uncapturedErrorCallbackInfo.callback = //
        [](WGPUDevice const* /*device*/,
           WGPUErrorType type,
           WGPUStringView msg,
           void* /*userdata1*/,
           void* /*userdata2*/) {
            fmt::println(
                "WebGPU device error: {} ({})\nMessage: {}",
                to_string(type),
                int(type),
                msg.data);
        };
    return result;
}

template <typename T>
T const* get_default()
{
    static T def = make_default<T>();
    return &def;
}

template <typename T>
T const* or_default(T const* obj)
{
    return obj ? obj : get_default<T>();
}

} // namespace

GpuContext GpuContext::make(
    WGPUInstanceDescriptor const* const instance_desc,
    WGPURequestAdapterOptions const* const adapter_opts,
    WGPUDeviceDescriptor const* const device_desc)
{
    WGPUInstance const instance = wgpuCreateInstance(or_default(instance_desc));
    assert(instance);

    WGPUAdapter const adapter = request_adapter(instance, or_default(adapter_opts));
    assert(adapter);

    WGPUDevice const device = request_device(instance, adapter, or_default(device_desc));
    assert(device);

    return {
        .instance = instance,
        .adapter = adapter,
        .device = device,
    };
}

GpuContext GpuContext::make(
    SurfaceSource const& surface_src,
    WGPUInstanceDescriptor const* const instance_desc,
    WGPURequestAdapterOptions const* const adapter_opts,
    WGPUDeviceDescriptor const* const device_desc)
{
    WGPUInstance const instance = wgpuCreateInstance(or_default(instance_desc));
    assert(instance);

    WGPUSurface const surface = make_surface(instance, surface_src);
    assert(surface);

    auto opts = *or_default(adapter_opts);
    opts.compatibleSurface = surface;
    WGPUAdapter const adapter = request_adapter(instance, &opts);
    assert(adapter);

    WGPUDevice const device = request_device(instance, adapter, or_default(device_desc));
    assert(device);

    GpuContext ctx{
        .instance = instance,
        .adapter = adapter,
        .device = device,
        .surface = surface,
    };
    ctx.config_surface(surface_src.window);
    return ctx;
}

void GpuContext::config_surface(i32 const width, i32 const height)
{
    WGPUSurfaceConfiguration config{};
    {
        config.device = device;
        config.width = width;
        config.height = height;
        config.format = default_surface_format;
        config.usage = WGPUTextureUsage_RenderAttachment;
        config.presentMode = default_surface_present_mode;
    }
    wgpuSurfaceConfigure(surface, &config);
}

void GpuContext::config_surface(GLFWwindow* const window)
{
    i32 width, height;
    glfwGetFramebufferSize(window, &width, &height);
    config_surface(width, height);
}

void GpuContext::report()
{
    report_adapter_features(adapter);
    report_adapter_limits(adapter);
    report_adapter_properties(adapter);
    report_device_features(device);
    report_device_limits(device);

    if (surface)
        report_surface_capabilities(surface, adapter);
}

} // namespace wgpu::sandbox