#include "graphics.h"

#include <assert.h>
#include <stdbool.h>

WGPUTextureView surface_make_view(WGPUSurface const surface)
{
    WGPUSurfaceTexture srf_tex;
    wgpuSurfaceGetCurrentTexture(surface, &srf_tex);
    assert(srf_tex.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal);

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
                    .clearValue = {.r = 0.15, .g = 0.15, .b = 0.15, .a = 1.0},
                    .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                },
        });
}

WGPURenderPipeline make_render_pipeline(
    WGPUDevice const device,
    WGPUStringView const shader_src,
    WGPUTextureFormat const color_format)
{
    // clang-format off
    WGPUVertexAttribute const vertex_attrs[] = {
        {
            .format = WGPUVertexFormat_Float32x2,
            .offset = 0,
            .shaderLocation = 0,
        },
        {
            .format = WGPUVertexFormat_Float32x2,
            .offset = sizeof(float[2]),
            .shaderLocation = 1,
        },
    };

    WGPUShaderModule const shader = wgpuDeviceCreateShaderModule(
        device, 
        &(WGPUShaderModuleDescriptor){
            .nextInChain = (WGPUChainedStruct const*)&(WGPUShaderSourceWGSL){
                .chain = {
                    .sType = WGPUSType_ShaderSourceWGSL,
                },
                .code = shader_src,
            },
        });

    WGPURenderPipeline const result = wgpuDeviceCreateRenderPipeline(
        device, 
        &(WGPURenderPipelineDescriptor){
            .vertex = {
                .module = shader,
                .entryPoint = {"vs_main", WGPU_STRLEN},
                .bufferCount = 1,
                .buffers = &(WGPUVertexBufferLayout){
                    .stepMode = WGPUVertexStepMode_Vertex,
                    .arrayStride = sizeof(float[4]),
                    .attributeCount = sizeof(vertex_attrs) / sizeof(*vertex_attrs),
                    .attributes = vertex_attrs,
                },
            },
            .primitive = {
                .topology = WGPUPrimitiveTopology_TriangleList,
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = WGPUCullMode_None,
            },
            .multisample = {
                .count = 1,
                .mask = ~0u,
                .alphaToCoverageEnabled = 0u,
            },
            .fragment = &(WGPUFragmentState){
                .module = shader,
                .entryPoint = {"fs_main", WGPU_STRLEN},
                .targetCount = 1,
                .targets = &(WGPUColorTargetState){
                    .format = color_format,
                    .writeMask = WGPUColorWriteMask_All,
                },
            },
        });
    // clang-format on

    wgpuShaderModuleRelease(shader);
    return result;
}

WGPUBuffer render_mesh_make_buffer(
    WGPUDevice const device,
    size_t const size,
    WGPUBufferUsage const usage)
{
    return wgpuDeviceCreateBuffer(
        device,
        &(WGPUBufferDescriptor){
            .usage = usage,
            .size = size,
            .mappedAtCreation = true,
        });
}
