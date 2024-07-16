#ifndef WGPU_CONFIG_INCLUDED
#define WGPU_CONFIG_INCLUDED

#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C"
{
#endif

WGPUTextureView make_texture_view(WGPUSurfaceTexture srf_texture);

WGPURenderPassEncoder begin_render_pass(WGPUCommandEncoder encoder, WGPUTextureView tex_view);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WGPU_CONFIG_INCLUDED
