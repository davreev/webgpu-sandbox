#pragma once

#include <webgpu/webgpu.h>

#include "gpu_resource.hpp"

namespace wgpu::sandbox
{

struct SurfaceRenderPass
{
    GpuTextureView view;
    GpuRenderPassEncoder encoder;

    SurfaceRenderPass(
        WGPUCommandEncoder cmd_encoder,
        WGPUSurface surface,
        WGPUColor const& clear_color = {0.15, 0.15, 0.15, 1.0},
        WGPUTextureView depth_view = {});
};

} // namespace wgpu::sandbox
