#pragma once

#include <wgpu_utils.hpp>

#include "dr_shim.hpp"

namespace wgpu::sandbox
{

inline constexpr WGPUTextureFormat default_surface_format = WGPUTextureFormat_BGRA8Unorm;
inline constexpr WGPUPresentMode default_surface_present_mode = WGPUPresentMode_Fifo;

struct GpuContext
{
    WGPUInstance instance;
    WGPUSurface surface;
    WGPUAdapter adapter;
    WGPUDevice device;

    static GpuContext make(
        WGPUInstanceDescriptor const* instance_desc = nullptr,
        WGPURequestAdapterOptions const* adapter_opts = nullptr,
        WGPUDeviceDescriptor const* device_desc = nullptr);

    static GpuContext make(
        SurfaceSource const& surface_src,
        WGPUInstanceDescriptor const* instance_desc = nullptr,
        WGPURequestAdapterOptions const* adapter_opts = nullptr,
        WGPUDeviceDescriptor const* device_desc = nullptr);

    static void release(GpuContext& ctx);

    void config_surface(int width, int height);
    void config_surface(GLFWwindow* window);

    void report();
};

struct MainLoop
{
    using Callback = void(void* userdata);
    WGPUSurface surface{};
    GLFWwindow* window{};
    Callback* callback{};
    void* userdata{};

    void begin() const;
};

} // namespace wgpu::sandbox
