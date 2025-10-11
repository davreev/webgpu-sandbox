#ifndef CONFIG_H
#define CONFIG_H

#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C"
{
#endif

WGPUTextureView surface_make_view(WGPUSurface surface);

WGPURenderPassEncoder render_pass_begin(WGPUCommandEncoder encoder, WGPUTextureView surface_view);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CONFIG_H
