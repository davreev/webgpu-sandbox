#include "passes.hpp"

#include <cassert>

namespace wgpu::sandbox
{
namespace
{

template <typename T>
struct Impl;

template <>
struct Impl<SurfaceRenderPass>
{
    static WGPUTextureView make_view(WGPUSurface const surface)
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

    static WGPURenderPassEncoder begin_pass(
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
};

template <>
struct Impl<DepthTarget>
{
    static WGPUTexture make_texture(
        WGPUDevice const device,
        uint32_t const width,
        uint32_t const height,
        WGPUTextureFormat const format)
    {
        WGPUTextureDescriptor const desc{
            .usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc,
            .dimension = WGPUTextureDimension_2D,
            .size = {width, height, 1},
            .format = format,
            .mipLevelCount = 1,
            .sampleCount = 1,
        };
        return wgpuDeviceCreateTexture(device, &desc);
    }

    static WGPUTextureView make_view(WGPUTexture const texture)
    {
        WGPUTextureViewDescriptor const desc{
            .format = wgpuTextureGetFormat(texture),
            .dimension = WGPUTextureViewDimension_2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
        };
        return wgpuTextureCreateView(texture, &desc);
    }
};

} // namespace

SurfaceRenderPass SurfaceRenderPass::make(
    WGPUCommandEncoder const cmd_encoder,
    WGPUSurface const surface,
    WGPUColor const& clear_color,
    WGPUTextureView const depth_view)
{
    using Impl = Impl<SurfaceRenderPass>;

    WGPUTextureView const view = Impl::make_view(surface);
    assert(view);

    WGPURenderPassEncoder const encoder = Impl::begin_pass(
        cmd_encoder,
        view,
        clear_color,
        depth_view);
    assert(encoder);

    return {
        .view = view,
        .encoder = encoder,
    };
}

DepthTarget DepthTarget::make(WGPUDevice const device, i32 const width, i32 const height)
{
    using Impl = Impl<DepthTarget>;

    WGPUTexture const texture = Impl::make_texture(device, width, height, format);
    assert(texture);

    WGPUTextureView const view = Impl::make_view(texture);
    assert(view);

    return {
        .texture = texture,
        .view = view,
    };
}

} // namespace wgpu::sandbox
