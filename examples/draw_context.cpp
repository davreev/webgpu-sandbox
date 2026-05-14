#include "draw_context.hpp"

#include <algorithm>
#include <cassert>

#include <wgpu_default_limits.hpp>

namespace wgpu::sandbox
{
namespace
{

enum struct BindIndex : u8
{
    Pass = 0,
    Material,
    Geometry,
    Object,
};

constexpr u32 uniform_buffer_alignment = default_min_uniform_buffer_offset_alignment;

GpuBindGroupLayout uniform_bg_layout{};

} // namespace

void check_device_limits(WGPUDevice const device)
{
    WGPULimits limits{};
    [[maybe_unused]]
    auto const status = wgpuDeviceGetLimits(device, &limits);
    assert(status == WGPUStatus::WGPUStatus_Success);
    assert(uniform_buffer_alignment >= limits.minUniformBufferOffsetAlignment);
}

void DrawContext::init_shared_resources(WGPUDevice const device)
{
    check_device_limits(device);

    WGPUBindGroupLayoutEntry const entries[1]{
        {
            .binding = 0,
            .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .buffer{
                .type = WGPUBufferBindingType_Uniform,
                .hasDynamicOffset = true,
                .minBindingSize = 0,
            },
        },
    };
    WGPUBindGroupLayoutDescriptor const desc{
        .entryCount = 1,
        .entries = entries,
    };
    uniform_bg_layout = wgpuDeviceCreateBindGroupLayout(device, &desc);
}

u32 DrawContext::push_uniforms(Span<u8 const> const& data)
{
    return uniform_stage_.append(data, uniform_buffer_alignment);
}

u32 DrawContext::push_uniforms_once(void const* key, Span<u8 const> const& data)
{
    auto const [it, ok] = uniform_offsets_.try_emplace(key);
    if (ok)
        it->second = push_uniforms(data);

    return it->second;
}

void DrawContext::rebuild_uniform_bg(WGPUDevice const device)
{
    assert(uniform_bg_layout);

    WGPUBindGroupEntry const entries[1]{
        {
            .binding = 0,
            .buffer = uniform_stage_.device_buf,
            .size = WGPU_WHOLE_SIZE,
        },
    };
    WGPUBindGroupDescriptor const desc{
        .layout = uniform_bg_layout,
        .entryCount = 1,
        .entries = entries,
    };
    uniform_bg_ = wgpuDeviceCreateBindGroup(device, &desc);
}

void DrawContext::submit_draw_cmds(
    WGPUDevice const device,
    WGPUQueue const queue,
    WGPURenderPassEncoder const encoder,
    WGPUBindGroup const pass_bg)
{
    if (uniform_stage_.update_device(device, queue, WGPUBufferUsage_Uniform))
        rebuild_uniform_bg(device);

    geometry.update_device_buffers(device, queue);

    // Order draw commands to minimize state changes
    std::sort(begin(draw_cmds), end(draw_cmds), [](DrawCommand const& a, DrawCommand const& b) {
        if (a.pipeline != b.pipeline)
            return a.pipeline < b.pipeline;
        else if (a.material_bg != b.material_bg)
            return a.material_bg < b.material_bg;
        else
            return a.geometry_bg < b.geometry_bg;
    });

    // Submit draw commands
    {
        WGPURenderPipeline prev_pipeline{};
        WGPUBindGroup prev_material_bg{};
        WGPUBindGroup prev_geometry_bg{};
        WGPUBuffer prev_index_buf{};

        if (pass_bg)
            wgpuRenderPassEncoderSetBindGroup(encoder, u32(BindIndex::Pass), pass_bg, 0, nullptr);

        for (auto const& cmd : draw_cmds)
        {
            if (cmd.pipeline != prev_pipeline)
            {
                wgpuRenderPassEncoderSetPipeline(encoder, cmd.pipeline);
                prev_pipeline = cmd.pipeline;
            }

            if (cmd.material_bg && cmd.material_bg != prev_material_bg)
            {
                wgpuRenderPassEncoderSetBindGroup(
                    encoder,
                    u32(BindIndex::Material),
                    cmd.material_bg,
                    0,
                    nullptr);

                prev_material_bg = cmd.material_bg;
            }

            // Always set the geometry bind group if using dynamic offsets as these will vary per
            // command
            if (cmd.geometry_offsets)
            {
                wgpuRenderPassEncoderSetBindGroup(
                    encoder,
                    u32(BindIndex::Geometry),
                    cmd.geometry_bg ? cmd.geometry_bg : geometry.bindings(),
                    4,
                    cmd.geometry_offsets.value());

                // Ignore prev bg when using dynamic offsets
                prev_geometry_bg = {};
            }
            else
            {
                if (cmd.geometry_bg && cmd.geometry_bg != prev_geometry_bg)
                {
                    wgpuRenderPassEncoderSetBindGroup(
                        encoder,
                        u32(BindIndex::Geometry),
                        cmd.geometry_bg,
                        0,
                        nullptr);

                    prev_geometry_bg = cmd.geometry_bg;
                }
            }

            bool const needs_index_buf = cmd.type == DrawCommand::Type::DrawIndexed
                || cmd.type == DrawCommand::Type::DrawIndexedIndirect;

            if (needs_index_buf)
            {
                WGPUBuffer index_buf{};
                WGPUIndexFormat index_fmt{};
                if (cmd.index_buf)
                {
                    index_buf = cmd.index_buf;
                    index_fmt = cmd.index_fmt;
                }
                else
                {
                    index_buf = geometry.index_buffer();
                    index_fmt = geometry.index_format;
                }
                assert(index_buf);

                if (index_buf != prev_index_buf)
                {
                    wgpuRenderPassEncoderSetIndexBuffer(
                        encoder,
                        index_buf,
                        index_fmt,
                        0,
                        WGPU_WHOLE_SIZE);

                    prev_index_buf = index_buf;
                }
            }

            if (uniform_bg_ && cmd.uniform_offset)
            {
                wgpuRenderPassEncoderSetBindGroup(
                    encoder,
                    u32(BindIndex::Object),
                    uniform_bg_,
                    1,
                    &cmd.uniform_offset.value());
            }

            switch (cmd.type)
            {
                case DrawCommand::Type::Draw:
                {
                    auto const& args = cmd.draw;
                    wgpuRenderPassEncoderDraw(
                        encoder,
                        args.vertex_count,
                        args.instance_count,
                        args.first_vertex,
                        args.first_instance);
                    break;
                };
                case DrawCommand::Type::DrawIndexed:
                {
                    auto const& args = cmd.draw_indexed;
                    wgpuRenderPassEncoderDrawIndexed(
                        encoder,
                        args.index_count,
                        args.instance_count,
                        args.first_index,
                        args.base_vertex,
                        args.first_instance);
                    break;
                };
                case DrawCommand::Type::DrawIndirect:
                {
                    auto const& args = cmd.draw_indirect;
                    wgpuRenderPassEncoderDrawIndirect(encoder, args.buffer, args.offset);
                    break;
                };
                case DrawCommand::Type::DrawIndexedIndirect:
                {
                    auto const& args = cmd.draw_indirect;
                    wgpuRenderPassEncoderDrawIndexedIndirect(encoder, args.buffer, args.offset);
                    break;
                };
                case DrawCommand::Type::Undefined:
                {
                    // Shouldn't be submitting undefined commands
                    assert(false);
                    break;
                };
            };
        }
    }

    // Cleanup
    draw_cmds.clear();
    geometry.clear();
    uniform_stage_.host_buf.clear();
    uniform_offsets_.clear();
}

} // namespace wgpu::sandbox
