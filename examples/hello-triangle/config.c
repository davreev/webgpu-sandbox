#include "config.h"

#include <assert.h>

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

    WGPURenderPipeline const result = wgpuDeviceCreateRenderPipeline(
        device, 
        &(WGPURenderPipelineDescriptor){
            .vertex = {
                .module = shader,
                .entryPoint = {"vs_main", WGPU_STRLEN},
            },
            .primitive = {
                .topology = WGPUPrimitiveTopology_TriangleList,
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = WGPUCullMode_None,
            },
            // .depthStencil = &(WGPUDepthStencilState){
            // },
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
                    // .blend = &(WGPUBlendState){
                    // },
                    .writeMask = WGPUColorWriteMask_All,
                },
            },
        });
    // clang-format on

    wgpuShaderModuleRelease(shader);
    return result;
}
