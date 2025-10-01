#include "utils.h"

WGPUBuffer make_buffer(WGPUDevice const device, size_t const size, WGPUBufferUsage const usage)
{
    return wgpuDeviceCreateBuffer(
        device,
        &(WGPUBufferDescriptor){
            .usage = usage,
            .size = size,
        });
}

WGPUComputePassEncoder compute_pass_begin(WGPUCommandEncoder const encoder)
{
    return wgpuCommandEncoderBeginComputePass(
        encoder,
        &(WGPUComputePassDescriptor){.timestampWrites = NULL});
}

WGPUBindGroupLayout unary_kernel_make_bind_group_layout(WGPUDevice const device)
{
    WGPUBindGroupLayoutEntry const entries[] = {
        {
            .binding = 0,
            .buffer.type = WGPUBufferBindingType_Storage,
            .visibility = WGPUShaderStage_Compute,
        },
    };

    return wgpuDeviceCreateBindGroupLayout(
        device,
        &(WGPUBindGroupLayoutDescriptor){
            .entries = entries,
            .entryCount = sizeof(entries) / sizeof(*entries),
        });
}

WGPUBindGroup unary_kernel_make_bind_group(
    WGPUDevice const device,
    WGPUBindGroupLayout const layout,
    WGPUBuffer const buffer)
{
    
    WGPUBindGroupEntry const entries[] = {
        {
            .binding = 0,
            .buffer = buffer,
            .size = wgpuBufferGetSize(buffer),
        },
    };

    return wgpuDeviceCreateBindGroup(
        device,
        &(WGPUBindGroupDescriptor){
            .layout = layout,
            .entries = entries,
            .entryCount = 1,
        });
}

WGPUPipelineLayout unary_kernel_make_pipeline_layout(
    WGPUDevice const device,
    WGPUBindGroupLayout const bind_group_layout)
{
    return wgpuDeviceCreatePipelineLayout(
        device,
        &(WGPUPipelineLayoutDescriptor){
            .bindGroupLayouts = &bind_group_layout,
            .bindGroupLayoutCount = 1,
        });
}

WGPUComputePipeline unary_kernel_make_pipeline(
    WGPUDevice const device,
    WGPUPipelineLayout const layout,
    WGPUStringView const shader_src)
{
    // clang-format off
    WGPUShaderModule const shader = wgpuDeviceCreateShaderModule(
        device,
        &(WGPUShaderModuleDescriptor){
            .nextInChain = (WGPUChainedStruct*)&(WGPUShaderSourceWGSL){
                .chain = {
                    .sType = WGPUSType_ShaderSourceWGSL,
                },
                .code = shader_src,
            },
        });

    WGPUComputePipeline const result = wgpuDeviceCreateComputePipeline(
        device, 
        &(WGPUComputePipelineDescriptor){
            .compute = {
                .module = shader,
                .entryPoint = {"compute_main", WGPU_STRLEN}
            },
            .layout = layout,
        });
    // clang-format on

    wgpuShaderModuleRelease(shader);
    return result;
}
