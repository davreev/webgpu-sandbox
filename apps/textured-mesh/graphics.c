#include "graphics.h"

#include <assert.h>
#include <stdbool.h>

WGPUTexture depth_target_make_texture(
    WGPUDevice const device,
    uint32_t const width,
    uint32_t const height,
    WGPUTextureFormat const format)
{
    return wgpuDeviceCreateTexture(
        device,
        &(WGPUTextureDescriptor){
            .usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc,
            .dimension = WGPUTextureDimension_2D,
            .size = {width, height, 1},
            .format = format,
            .mipLevelCount = 1,
            .sampleCount = 1,
        });
}

WGPUTextureView depth_target_make_view(WGPUTexture const texture)
{
    return wgpuTextureCreateView(
        texture,
        &(WGPUTextureViewDescriptor){
            .format = wgpuTextureGetFormat(texture),
            .dimension = WGPUTextureViewDimension_2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
        });
}

WGPUBindGroupLayout render_material_make_bind_group_layout(WGPUDevice const device)
{
    // clang-format off
    WGPUBindGroupLayoutEntry entries[] = {
        {
            // Texture
            .binding = 0,
            .visibility = WGPUShaderStage_Fragment,
            .texture = {
                .sampleType = WGPUTextureSampleType_Float,
                .viewDimension = WGPUTextureViewDimension_2D,
                .multisampled = false,
            },
        },
        {
            // Sampler
            .binding = 1,
            .visibility = WGPUShaderStage_Fragment,
            .sampler = {
                .type = WGPUSamplerBindingType_Filtering,
            },
        },
        {
            // Uniforms
            .binding = 2,
            .visibility = WGPUShaderStage_Vertex,
            .buffer = {
                .type = WGPUBufferBindingType_Uniform,
                .hasDynamicOffset = false,
                .minBindingSize = 0,
            },
        },
    };
    // clang-format on

    return wgpuDeviceCreateBindGroupLayout(
        device,
        &(WGPUBindGroupLayoutDescriptor){
            .entryCount = sizeof(entries) / sizeof(*entries),
            .entries = entries,
        });
}

WGPUBindGroup render_material_make_bind_group(
    WGPUDevice const device,
    WGPUBindGroupLayout const layout,
    WGPUTextureView const color_view,
    WGPUSampler const color_sampler,
    WGPUBuffer const uniforms)
{
    // clang-format off
    WGPUBindGroupEntry const entries[] = {
        {
            .binding = 0,
            .textureView = color_view,
        },
        {
            .binding = 1,
            .sampler = color_sampler,
        },
        {
            .binding = 2,
            .buffer = uniforms,
            .size = wgpuBufferGetSize(uniforms),
        },
    };
    // clang-format on

    return wgpuDeviceCreateBindGroup(
        device,
        &(WGPUBindGroupDescriptor){
            .layout = layout,
            .entryCount = sizeof(entries) / sizeof(*entries),
            .entries = entries,
        });
}

WGPUPipelineLayout render_material_make_pipeline_layout(
    WGPUDevice const device,
    WGPUBindGroupLayout const bind_group_layout)
{
    return wgpuDeviceCreatePipelineLayout(
        device,
        &(WGPUPipelineLayoutDescriptor){
            .bindGroupLayoutCount = 1,
            .bindGroupLayouts = &bind_group_layout,
        });
}

WGPURenderPipeline render_material_make_pipeline(
    WGPUDevice const device,
    WGPUPipelineLayout const layout,
    WGPUStringView const shader_src,
    WGPUTextureFormat const surface_format,
    WGPUTextureFormat const depth_format)
{
    WGPUVertexAttribute const attrs[] = {
        {
            .format = WGPUVertexFormat_Float32x3,
            .offset = 0,
            .shaderLocation = 0,
        },
        {
            .format = WGPUVertexFormat_Float32x2,
            .offset = sizeof(float[3]),
            .shaderLocation = 1,
        },
    };

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
            .layout = layout,
            .vertex = {
                .module = shader,
                .entryPoint = {"vs_main", WGPU_STRLEN},
                .bufferCount = 1,
                .buffers = &(WGPUVertexBufferLayout){
                    .stepMode = WGPUVertexStepMode_Vertex,
                    .arrayStride = sizeof(float[5]),
                    .attributeCount = sizeof(attrs) / sizeof(*attrs),
                    .attributes = attrs,
                },
            },
            .primitive = {
                .topology = WGPUPrimitiveTopology_TriangleList,
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = WGPUCullMode_None,
            },
            .depthStencil = &(WGPUDepthStencilState){
                .format = depth_format,
                .depthWriteEnabled = true,
                .depthCompare = WGPUCompareFunction_LessEqual,
#ifndef __EMSCRIPTEN__
                // NOTE(dr): As of v0.19.4.1, wgpu-native appears to use WGPUCompareFunction_Never
                // as the default. This isn't consistent with the WebGPU standard and causes a panic
                // at runtime. As a workaround, setting the standard-compliant default appears to do
                // the trick. See related (closed?) issue:
                // https://github.com/gfx-rs/wgpu-native/issues/40
                .stencilFront = {
                    .compare = WGPUCompareFunction_Always,
                },
                .stencilBack = {
                    .compare = WGPUCompareFunction_Always,
                },
#endif
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
                    .format = surface_format,
                    .writeMask = WGPUColorWriteMask_All,
                },
            },
        });
    // clang-format on

    wgpuShaderModuleRelease(shader);
    return result;
}

WGPUTexture render_material_make_color_texture(
    WGPUDevice const device,
    void* const data,
    uint32_t const width,
    uint32_t const height,
    uint32_t const stride,
    WGPUTextureView* view,
    WGPUSampler* sampler)
{
    WGPUTextureFormat const format = WGPUTextureFormat_RGBA8Unorm;
    WGPUExtent3D const size = {width, height, 1};
    uint32_t const row_size = width * stride;
    uint32_t const data_size = row_size * height;

    WGPUTexture const texture = wgpuDeviceCreateTexture(
        device,
        &(WGPUTextureDescriptor){
            .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
            .dimension = WGPUTextureDimension_2D,
            .size = size,
            .format = format,
            .mipLevelCount = 1,
            .sampleCount = 1,
        });

    // Copy image data to mip level 0
    WGPUQueue const queue = wgpuDeviceGetQueue(device);
    wgpuQueueWriteTexture(
        queue,
        &(WGPUTexelCopyTextureInfo){
            .texture = texture,
        },
        data,
        data_size,
        &(WGPUTexelCopyBufferLayout){
            .bytesPerRow = row_size,
            .rowsPerImage = height,
        },
        &size);

    *view = wgpuTextureCreateView(
        texture,
        &(WGPUTextureViewDescriptor){
            .format = format,
            .dimension = WGPUTextureViewDimension_2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
        });

    *sampler = wgpuDeviceCreateSampler(
        device,
        &(WGPUSamplerDescriptor){
            .magFilter = WGPUFilterMode_Nearest,
            .minFilter = WGPUFilterMode_Nearest,
            .maxAnisotropy = 1,
        });

    return texture;
}

WGPUBuffer render_material_make_uniform_buffer(WGPUDevice const device, size_t const size)
{
    return wgpuDeviceCreateBuffer(
        device,
        &(WGPUBufferDescriptor){
            .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
            .size = size,
        });
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
    WGPUTextureView const surface_view,
    WGPUTextureView const depth_view)
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
            .depthStencilAttachment = &(WGPURenderPassDepthStencilAttachment){
                .view = depth_view,
                .depthLoadOp = WGPULoadOp_Clear,
                .depthStoreOp = WGPUStoreOp_Store,
                .depthClearValue = 1.0f,
            }});
}
