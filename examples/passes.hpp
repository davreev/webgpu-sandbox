#pragma once

#include <webgpu/webgpu.h>

#include "basic_types.hpp"
#include "gpu_resource.hpp"

namespace wgpu::sandbox
{

struct SurfaceRenderPass
{
    GpuTextureView view;
    GpuRenderPassEncoder encoder;
    
    static SurfaceRenderPass make(
        WGPUCommandEncoder cmd_encoder,
        WGPUSurface surface,
        WGPUColor const& clear_color = {0.15, 0.15, 0.15, 1.0},
        WGPUTextureView depth_view = {});
};

struct DepthTarget
{
    static constexpr WGPUTextureFormat format = WGPUTextureFormat_Depth32Float;
    GpuTexture texture;
    GpuTextureView view;

    static DepthTarget make(WGPUDevice device, i32 width, i32 height);
};

} // namespace wgpu::sandbox
