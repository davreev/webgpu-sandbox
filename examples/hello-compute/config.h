#ifndef CONFIG_H
#define CONFIG_H

#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C"
{
#endif

WGPUBuffer make_buffer(WGPUDevice device, size_t size, WGPUBufferUsage usage);

WGPUComputePassEncoder compute_pass_begin(WGPUCommandEncoder encoder);

WGPUBindGroupLayout unary_kernel_make_bind_group_layout(WGPUDevice device);

WGPUBindGroup unary_kernel_make_bind_group(
    WGPUDevice device,
    WGPUBindGroupLayout layout,
    WGPUBuffer buffer);

WGPUPipelineLayout unary_kernel_make_pipeline_layout(
    WGPUDevice device,
    WGPUBindGroupLayout bind_group_layout);

WGPUComputePipeline unary_kernel_make_pipeline(
    WGPUDevice device,
    WGPUPipelineLayout layout,
    WGPUStringView shader_src);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CONFIG_H
