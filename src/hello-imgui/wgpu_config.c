#include "wgpu_config.h"

WGPUTextureView make_texture_view(WGPUSurfaceTexture const srf_texture)
{
    WGPUTextureViewDescriptor const desc = {
        .mipLevelCount = 1,
        .arrayLayerCount = 1,
    };
    return wgpuTextureCreateView(srf_texture.texture, &desc);
}

WGPURenderPassEncoder begin_render_pass(
    WGPUCommandEncoder const encoder,
    WGPUTextureView const tex_view,
    WGPUColor const* clear_color)
{
    static WGPUColor const default_clear_color = {.r = 0.15, .g = 0.15, .b = 0.15, .a = 1.0};

    // clang-format off
    WGPURenderPassDescriptor const desc = {
        .colorAttachmentCount = 1,
        .colorAttachments = &(WGPURenderPassColorAttachment){
            .view = tex_view,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue = clear_color ? *clear_color : default_clear_color,
#ifdef __EMSCRIPTEN__
            // NOTE(dr): This isn't defined in wgpu-native yet (https://github.com/eliemichel/WebGPU-distribution/issues/140)
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED
#endif
        },
    };
    // clang-format on

    return wgpuCommandEncoderBeginRenderPass(encoder, &desc);
}
