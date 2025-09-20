#include "example_base.hpp"

#include <cassert>

#include <fmt/core.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

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
            fmt::print(
                "WebGPU device error: {} ({})\nMessage: {}\n",
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
    GpuContext result{};

    result.instance = wgpuCreateInstance(or_default(instance_desc));
    assert(result.instance);

    result.adapter = request_adapter(result.instance, or_default(adapter_opts));
    assert(result.adapter);

    result.device = request_device(result.instance, result.adapter, or_default(device_desc));
    assert(result.device);

    return result;
}

GpuContext GpuContext::make(
    SurfaceSource const& surface_src,
    WGPUInstanceDescriptor const* const instance_desc,
    WGPURequestAdapterOptions const* const adapter_opts,
    WGPUDeviceDescriptor const* const device_desc)
{
    GpuContext result{};

    result.instance = wgpuCreateInstance(or_default(instance_desc));
    assert(result.instance);

    result.surface = make_surface(result.instance, surface_src);
    assert(result.surface);

    auto opts = *or_default(adapter_opts);
    opts.compatibleSurface = result.surface;
    result.adapter = request_adapter(result.instance, &opts);
    assert(result.adapter);

    result.device = request_device(result.instance, result.adapter, or_default(device_desc));
    assert(result.device);

    result.config_surface(surface_src.window);

    return result;
}

void GpuContext::release(GpuContext& ctx)
{
    if (ctx.surface)
    {
        wgpuSurfaceUnconfigure(ctx.surface);
        wgpuSurfaceRelease(ctx.surface);
    }
    wgpuDeviceRelease(ctx.device);
    wgpuAdapterRelease(ctx.adapter);
    wgpuInstanceRelease(ctx.instance);
    ctx = {};
}

void GpuContext::config_surface(int const width, int const height)
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
    int width, height;
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
    report_surface_capabilities(surface, adapter);
}

void MainLoop::begin() const
{
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(callback, userdata, 0, true);
#else
    while (!glfwWindowShouldClose(window))
    {
        callback(userdata);
        wgpuSurfacePresent(surface);
    }
#endif
}

} // namespace wgpu::sandbox