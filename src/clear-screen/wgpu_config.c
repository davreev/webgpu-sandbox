#include "wgpu_config.h"

WGPUTextureView make_texture_view(WGPUSurfaceTexture const srf_texture)
{
    return wgpuTextureCreateView(
        srf_texture.texture,
        &(WGPUTextureViewDescriptor){
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
        });
}

WGPURenderPassEncoder begin_render_pass(
    WGPUCommandEncoder const encoder,
    WGPUTextureView const tex_view)
{
    return wgpuCommandEncoderBeginRenderPass(
        encoder,
        &(WGPURenderPassDescriptor){
            .colorAttachmentCount = 1,
            .colorAttachments =
                &(WGPURenderPassColorAttachment){
                    .view = tex_view,
                    .loadOp = WGPULoadOp_Clear,
                    .storeOp = WGPUStoreOp_Store,
                    .clearValue = {.r = 1.0, .g = 0.0, .b = 0.5, .a = 1.0},
#ifdef __EMSCRIPTEN__
                    // NOTE(dr): This isn't defined in wgpu-native yet
                    // (https://github.com/eliemichel/WebGPU-distribution/issues/140)
                    .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED
#endif
                },
        });
}
