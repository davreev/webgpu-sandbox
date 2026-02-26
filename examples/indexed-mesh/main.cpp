#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include <dr/basic_types.hpp>
#include <dr/container_utils.hpp>
#include <dr/defer.hpp>
#include <dr/memory.hpp>
#include <dr/span.hpp>

#include "shader_src.hpp"

#include "../example_app.hpp"

namespace wgpu::sandbox
{
namespace
{

using App = ExampleApp;

struct RenderPass
{
    WGPURenderPassEncoder encoder;
    WGPUTextureView surface_view;

    static RenderPass begin(WGPUCommandEncoder const cmd_encoder, WGPUSurface const surface)
    {
        RenderPass result{};

        result.surface_view = make_view(surface);
        assert(result.surface_view);

        result.encoder = begin(cmd_encoder, result.surface_view);
        assert(result.encoder);

        return result;
    }

    static void end(RenderPass& pass)
    {
        wgpuRenderPassEncoderEnd(pass.encoder);
        wgpuRenderPassEncoderRelease(pass.encoder);
        wgpuTextureViewRelease(pass.surface_view);
        pass = {};
    }

  private:
    static WGPUTextureView make_view(WGPUSurface const surface)
    {
        WGPUSurfaceTexture srf_tex;
        wgpuSurfaceGetCurrentTexture(surface, &srf_tex);
        assert(srf_tex.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal);

        WGPUTextureViewDescriptor const desc{
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
        };
        return wgpuTextureCreateView(srf_tex.texture, &desc);
    }

    static WGPURenderPassEncoder begin(
        WGPUCommandEncoder const encoder,
        WGPUTextureView const surface_view)
    {
        WGPURenderPassColorAttachment color_atts[]{
            {
                .view = surface_view,
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue{0.15, 0.15, 0.15, 1.0},
            },
        };
        WGPURenderPassDescriptor const desc{
            .colorAttachmentCount = 1,
            .colorAttachments = color_atts,
        };
        return wgpuCommandEncoderBeginRenderPass(encoder, &desc);
    }
};

struct RenderMesh
{
    static constexpr WGPUIndexFormat index_format{WGPUIndexFormat_Uint16};
    WGPUBuffer vertices;
    WGPUBuffer indices;
    isize index_count;

    static RenderMesh make(
        WGPUDevice const device,
        Span<u8 const> const& vertex_data,
        Span<u8 const> const& index_data)
    {
        RenderMesh result{};

        // Create buffers
        result.vertices = make_buffer(
            device,
            vertex_data.size(),
            WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst);
        assert(result.vertices);

        result.indices = make_buffer(
            device,
            index_data.size(),
            WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst);
        assert(result.indices);

        auto const unmap = defer([&]() {
            wgpuBufferUnmap(result.vertices);
            wgpuBufferUnmap(result.indices);
        });

        // Copy data to buffers
        auto const copy_data = [](WGPUBuffer const dst, Span<u8 const> const& src) {
            void* dst_ptr = wgpuBufferGetMappedRange(dst, 0, src.size());
            assert(dst_ptr);
            std::memcpy(dst_ptr, src.data(), src.size());
        };
        copy_data(result.vertices, vertex_data);
        copy_data(result.indices, index_data);

        constexpr i8 index_stride = sizeof(u16);
        result.index_count = index_data.size() / index_stride;

        return result;
    }

    static RenderMesh make_quad(WGPUDevice const device)
    {
        // Format: x, y, z, u, v
        static constexpr f32 vertices[][4]{
            {-0.5, -0.5, 0.0, 0.0},
            {0.5, -0.5, 1.0, 0.0},
            {-0.5, 0.5, 0.0, 1.0},
            {0.5, 0.5, 1.0, 1.0},
        };

        static constexpr u16 faces[][3]{
            {0, 1, 2},
            {3, 2, 1},
        };

        return make(device, as<u8>(as_span(vertices)), as<u8>(as_span(faces)));
    }

    static void release(RenderMesh& mesh)
    {
        wgpuBufferRelease(mesh.vertices);
        wgpuBufferRelease(mesh.indices);
        mesh = {};
    }

    void bind_resources(WGPURenderPassEncoder const encoder)
    {
        wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, vertices, 0, wgpuBufferGetSize(vertices));
        wgpuRenderPassEncoderSetIndexBuffer(
            encoder,
            indices,
            index_format,
            0,
            wgpuBufferGetSize(indices));
    }

    void dispatch_draw(WGPURenderPassEncoder const encoder) const
    {
        wgpuRenderPassEncoderDrawIndexed(encoder, index_count, 1, 0, 0, 0);
    }

  private:
    static WGPUBuffer make_buffer(
        WGPUDevice const device,
        size_t const size,
        WGPUBufferUsage const usage)
    {
        WGPUBufferDescriptor const desc{
            .usage = usage,
            .size = size,
            .mappedAtCreation = true,
        };
        return wgpuDeviceCreateBuffer(device, &desc);
    }
};

struct
{
    WGPURenderPipeline pipeline{};
    RenderMesh geometry;
} state;

WGPURenderPipeline make_render_pipeline(
    WGPUDevice const device,
    WGPUStringView const shader_src,
    WGPUTextureFormat const color_format)
{
    WGPUShaderSourceWGSL shader_desc_src{
        .chain = {.sType = WGPUSType_ShaderSourceWGSL},
        .code = shader_src,
    };
    WGPUShaderModuleDescriptor const shader_desc{
        .nextInChain = as<WGPUChainedStruct>(&shader_desc_src),
    };
    WGPUShaderModule const shader = wgpuDeviceCreateShaderModule(device, &shader_desc);
    auto const drop_shader = defer([=]() { wgpuShaderModuleRelease(shader); });

    WGPUVertexAttribute const vert_attrs[]{
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
    WGPUVertexBufferLayout const vert_buf_layout{
        .stepMode = WGPUVertexStepMode_Vertex,
        .arrayStride = sizeof(float[4]),
        .attributeCount = size(vert_attrs),
        .attributes = vert_attrs,
    };

    WGPUColorTargetState const color_targ{
        .format = color_format,
        .writeMask = WGPUColorWriteMask_All,
    };
    WGPUFragmentState const frag_state{
        .module = shader,
        .entryPoint = {"fs_main", WGPU_STRLEN},
        .targetCount = 1,
        .targets = &color_targ,
    };
    WGPURenderPipelineDescriptor const pipe_desc{
        .vertex{
            .module = shader,
            .entryPoint{"vs_main", WGPU_STRLEN},
            .bufferCount = 1,
            .buffers = &vert_buf_layout,
        },
        .primitive{
            .topology = WGPUPrimitiveTopology_TriangleList,
            .frontFace = WGPUFrontFace_CCW,
            .cullMode = WGPUCullMode_None,
        },
        .multisample{
            .count = 1,
            .mask = ~0u,
            .alphaToCoverageEnabled = 0u,
        },
        .fragment = &frag_state,
    };

    return wgpuDeviceCreateRenderPipeline(device, &pipe_desc);
}

void init()
{
    // Create render pipeline
    state.pipeline = make_render_pipeline(
        App::gpu().device,
        {shader_src, WGPU_STRLEN},
        default_surface_format);

    // Create geometry
    state.geometry = RenderMesh::make_quad(App::gpu().device);
}

void deinit()
{
    RenderMesh::release(state.geometry);
    wgpuRenderPipelineRelease(state.pipeline);
    state = {};
}

void update()
{
    // Create a command encoder from the device
    WGPUCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(
        App::gpu().device,
        nullptr);
    assert(cmd_encoder);
    auto const drop_cmd_encoder = defer([=]() { wgpuCommandEncoderRelease(cmd_encoder); });

    // Render pass
    {
        RenderPass pass = RenderPass::begin(cmd_encoder, App::gpu().surface);
        auto const end_pass = defer([&]() { RenderPass::end(pass); });

        wgpuRenderPassEncoderSetPipeline(pass.encoder, state.pipeline);
        state.geometry.bind_resources(pass.encoder);
        state.geometry.dispatch_draw(pass.encoder);
    }

    // Create encoded commands
    WGPUCommandBuffer const cmds = wgpuCommandEncoderFinish(cmd_encoder, nullptr);
    assert(cmds);
    auto const drop_cmds = defer([=]() { wgpuCommandBufferRelease(cmds); });

    // Submit encoded commands
    WGPUQueue const queue = wgpuDeviceGetQueue(App::gpu().device);
    wgpuQueueSubmit(queue, 1, &cmds);
}

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    App::run({
        .init_cb = init,
        .frame_cb = update,
        .deinit_cb = deinit,
        .window{
            .title = "WebGPU Sandbox: Indexed Mesh",
            .width = 800,
            .height = 600,
        },
        .html_canvas_id = "#indexed-mesh",
    });

    return 0;
}
