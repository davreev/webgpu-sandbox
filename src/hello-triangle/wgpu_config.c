#include "wgpu_config.h"

WGPUShaderModule make_shader_module(WGPUDevice const device, char const* const shader_src)
{
    // clang-format off
    return wgpuDeviceCreateShaderModule(
        device,
        &(WGPUShaderModuleDescriptor){
            .nextInChain = (WGPUChainedStruct const*)&(WGPUShaderModuleWGSLDescriptor){
                .chain = {
                    .sType = WGPUSType_ShaderModuleWGSLDescriptor,
                },
                .code = shader_src,
            },
        });
    // clang-format on
}

WGPURenderPipeline make_render_pipeline(
    WGPUDevice const device,
    WGPUShaderModule const shader_module,
    WGPUTextureFormat const color_format)
{
    // clang-format off
    return wgpuDeviceCreateRenderPipeline(
        device, 
        &(WGPURenderPipelineDescriptor){
            .vertex = {
                .module = shader_module,
                .entryPoint = "vs_main",
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
                .module = shader_module,
                .entryPoint = "fs_main",
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
}

WGPUTextureView make_texture_view(WGPUSurfaceTexture const srf_texture)
{
    return wgpuTextureCreateView(
        srf_texture.texture,
        &(WGPUTextureViewDescriptor){
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
        });
}

WGPURenderPassEncoder begin_render_pass(
    WGPUCommandEncoder const encoder,
    WGPUTextureView const tex_view)
{
    // clang-format off
    return wgpuCommandEncoderBeginRenderPass(
        encoder, 
        &(WGPURenderPassDescriptor){
            .colorAttachmentCount = 1,
            .colorAttachments = &(WGPURenderPassColorAttachment){
                .view = tex_view,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue = { .r = 0.15, .g = 0.15, .b = 0.15, .a = 1.0},
#ifdef __EMSCRIPTEN__
                // NOTE(dr): This isn't defined in wgpu-native yet (https://github.com/eliemichel/WebGPU-distribution/issues/140)
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED
#endif
            },
        });
    // clang-format on
}
