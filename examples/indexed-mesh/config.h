#ifndef CONFIG_H
#define CONFIG_H

#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C"
{
#endif

WGPUTextureView surface_make_view(WGPUSurface surface);

WGPURenderPassEncoder render_pass_begin(WGPUCommandEncoder encoder, WGPUTextureView surface_view);

WGPURenderPipeline make_render_pipeline(
    WGPUDevice device,
    WGPUStringView shader_src,
    WGPUTextureFormat color_format);

WGPUBuffer render_mesh_make_buffer(WGPUDevice device, size_t size, WGPUBufferUsage usage);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CONFIG_H
