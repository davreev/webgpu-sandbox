#include "surface_render_pass.hpp"

#include <cassert>

namespace wgpu::sandbox
{
namespace
{

WGPUTextureView make_view(WGPUSurface const surface)
{
    WGPUSurfaceTexture srf_tex;
    wgpuSurfaceGetCurrentTexture(surface, &srf_tex);
    assert(srf_tex.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal);

    WGPUTextureViewDescriptor const desc{
        .mipLevelCount = 1,
        .arrayLayerCount = 1,
    };
    return wgpuTextureCreateView(srf_tex.texture, &desc);
}

WGPURenderPassEncoder begin_pass(
    WGPUCommandEncoder const cmd_encoder,
    WGPUTextureView const surface_view,
    WGPUColor const& clear_color,
    WGPUTextureView const depth_view)
{
    WGPURenderPassColorAttachment color_atts[]{
        {
            .view = surface_view,
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue = clear_color,
        },
    };

    WGPURenderPassDepthStencilAttachment const depth_att{
        .view = depth_view,
        .depthLoadOp = WGPULoadOp_Clear,
        .depthStoreOp = WGPUStoreOp_Store,
        .depthClearValue = 1.0f,
    };

    WGPURenderPassDescriptor const desc{
        .colorAttachmentCount = 1,
        .colorAttachments = color_atts,
        .depthStencilAttachment = depth_view ? &depth_att : nullptr,
    };
    return wgpuCommandEncoderBeginRenderPass(cmd_encoder, &desc);
}

} // namespace

SurfaceRenderPass::SurfaceRenderPass(
    WGPUCommandEncoder const cmd_encoder,
    WGPUSurface const surface,
    WGPUColor const& clear_color,
    WGPUTextureView const depth_view)
{
    view = make_view(surface);
    assert(view);

    encoder = begin_pass(cmd_encoder, view, clear_color, depth_view);
    assert(encoder);
}

} // namespace wgpu::sandbox
