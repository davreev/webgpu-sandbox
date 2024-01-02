#include "gfx_config.h"

#include <assert.h>

WGPUShaderModule make_shader_module(WGPUDevice const device, char const* const shader_src)
{
    // clang-format off
    WGPUShaderModuleWGSLDescriptor const next_desc = {
        .code = shader_src,
        .chain.sType = WGPUSType_ShaderModuleWGSLDescriptor,
    };

    WGPUShaderModuleDescriptor const desc = {
        .nextInChain = (WGPUChainedStruct const*)&next_desc,
    };
    // clang-format on

    return wgpuDeviceCreateShaderModule(device, &desc);
}

WGPURenderPipeline make_render_pipeline(
    WGPUDevice const device,
    WGPUShaderModule const shader_module,
    WGPUTextureFormat const color_format)
{
    // clang-format off
    WGPURenderPipelineDescriptor const desc = {
        .vertex = {
            .module = shader_module,
            .entryPoint = "vs_main",
        },
        .primitive = {
            .topology = WGPUPrimitiveTopology_TriangleList,
            .frontFace = WGPUFrontFace_CCW,
            .cullMode = WGPUCullMode_None,
        },
        .fragment = &(WGPUFragmentState){
            .module = shader_module,
            .entryPoint = "fs_main",
            .targetCount = 1,
            .targets = &(WGPUColorTargetState){
                .format = color_format,
                .writeMask = WGPUColorWriteMask_All,
                .blend = &(WGPUBlendState){
                    .color = {
                        .srcFactor = WGPUBlendFactor_SrcAlpha,
                        .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
                        .operation = WGPUBlendOperation_Add,
                    },
                    .alpha = {
                        .srcFactor = WGPUBlendFactor_Zero,
                        .dstFactor = WGPUBlendFactor_One,
                        .operation = WGPUBlendOperation_Add,
                    },
                },
            },
        },
        .multisample = {
            .count = 1,
            .mask = ~0u,
            .alphaToCoverageEnabled = 0u,
        },
        // .depthStencil = &(WGPUDepthStencilState){
        //     // ...
        // },
        // .layout = {
        //     // ...
        // }.
    };
    // clang-format on

    return wgpuDeviceCreateRenderPipeline(device, &desc);
}

WGPUTextureView make_texture_view(WGPUSurfaceTexture const srf_texture)
{
    WGPUTextureViewDescriptor const desc = {
        .mipLevelCount = 1,
        .arrayLayerCount = 1,
    };
    return wgpuTextureCreateView(srf_texture.texture, &desc);
}

WGPURenderPassEncoder begin_render_pass(WGPUCommandEncoder const encoder, WGPUTextureView const tex_view)
{
    // clang-format off
    WGPURenderPassDescriptor const desc = {
        .colorAttachmentCount = 1,
        .colorAttachments = &(WGPURenderPassColorAttachment){
            .view = tex_view,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue = { .r = 1.0, .g = 0.3, .b = 0.3, .a = 1.0},
        }
    };
    // clang-format on

    return wgpuCommandEncoderBeginRenderPass(encoder, &desc);
}
