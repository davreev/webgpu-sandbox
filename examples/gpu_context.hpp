#pragma once

#include <dr/basic_types.hpp>

#include <wgpu_utils.hpp>

#include "gpu_resource.hpp"

#include "dr_shim.hpp"

namespace wgpu::sandbox
{

inline constexpr WGPUTextureFormat default_surface_format = WGPUTextureFormat_BGRA8Unorm;
inline constexpr WGPUPresentMode default_surface_present_mode = WGPUPresentMode_Fifo;

struct GpuContext
{
    GpuInstance instance;
    GpuAdapter adapter;
    GpuDevice device;
    GpuSurface surface;

    static GpuContext make(
        WGPUInstanceDescriptor const* instance_desc = nullptr,
        WGPURequestAdapterOptions const* adapter_opts = nullptr,
        WGPUDeviceDescriptor const* device_desc = nullptr);

    static GpuContext make(
        SurfaceSource const& surface_src,
        WGPUInstanceDescriptor const* instance_desc = nullptr,
        WGPURequestAdapterOptions const* adapter_opts = nullptr,
        WGPUDeviceDescriptor const* device_desc = nullptr);

    void config_surface(i32 width, i32 height);
    void config_surface(GLFWwindow* window);

    void report();
};

} // namespace wgpu::sandbox