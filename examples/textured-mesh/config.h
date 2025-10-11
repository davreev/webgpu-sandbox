#ifndef CONFIG_H
#define CONFIG_H

#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C"
{
#endif

WGPUTexture depth_target_make_texture(
    WGPUDevice device,
    uint32_t width,
    uint32_t height,
    WGPUTextureFormat format);

WGPUTextureView depth_target_make_view(WGPUTexture texture);

WGPUBindGroupLayout render_material_make_bind_group_layout(WGPUDevice device);

WGPUBindGroup render_material_make_bind_group(
    WGPUDevice device,
    WGPUBindGroupLayout layout,
    WGPUTextureView color_view,
    WGPUSampler color_sampler,
    WGPUBuffer uniforms);

WGPUPipelineLayout render_material_make_pipeline_layout(
    WGPUDevice device,
    WGPUBindGroupLayout bind_group_layout);

WGPURenderPipeline render_material_make_pipeline(
    WGPUDevice device,
    WGPUPipelineLayout layout,
    WGPUStringView shader_src,
    WGPUTextureFormat surface_format,
    WGPUTextureFormat depth_format);

WGPUTexture render_material_make_color_texture(
    WGPUDevice device,
    void* data,
    uint32_t width,
    uint32_t height,
    uint32_t stride,
    WGPUTextureView* view,
    WGPUSampler* sampler);

WGPUBuffer render_material_make_uniform_buffer(WGPUDevice device, size_t size);

WGPUBuffer render_mesh_make_buffer(WGPUDevice device, size_t size, WGPUBufferUsage usage);

WGPUTextureView surface_make_view(WGPUSurface surface);

WGPURenderPassEncoder render_pass_begin(
    WGPUCommandEncoder encoder,
    WGPUTextureView surface_view,
    WGPUTextureView depth_view);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CONFIG_H
