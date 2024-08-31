#include "graphics.h"

#include <assert.h>

WGPUTextureView surface_make_view(WGPUSurface const surface)
{
    WGPUSurfaceTexture srf_tex;
    wgpuSurfaceGetCurrentTexture(surface, &srf_tex);
    assert(srf_tex.status == WGPUSurfaceGetCurrentTextureStatus_Success);

    return wgpuTextureCreateView(
        srf_tex.texture,
        &(WGPUTextureViewDescriptor){
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
        });
}

WGPURenderPassEncoder render_pass_begin(
    WGPUCommandEncoder const encoder,
    WGPUTextureView const surface_view)
{
    return wgpuCommandEncoderBeginRenderPass(
        encoder,
        &(WGPURenderPassDescriptor){
            .colorAttachmentCount = 1,
            .colorAttachments =
                &(WGPURenderPassColorAttachment){
                    .view = surface_view,
                    .loadOp = WGPULoadOp_Clear,
                    .storeOp = WGPUStoreOp_Store,
                    .clearValue = {.r = 1.0, .g = 0.0, .b = 0.5, .a = 1.0},
#ifdef __EMSCRIPTEN__
                    // NOTE(dr): This isn't defined in wgpu-native as of v0.19.4.1 but is required
                    // for web builds. See related issue:
                    // https://github.com/eliemichel/WebGPU-distribution/issues/14
                    .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED
#endif
                },
        });
}
