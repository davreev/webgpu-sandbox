#include "draw_context.hpp"

#include <algorithm>
#include <cassert>

#include <wgpu_default_limits.hpp>

namespace wgpu::sandbox
{

void DrawContext::submit_draw_cmds(
    WGPUDevice const device,
    WGPUQueue const queue,
    WGPURenderPassEncoder const encoder,
    PassInfo const& pass)
{
    // Push pass uniforms
    u32 const pass_uniform_offset = uniform_stream.push(pass.uniform_data);

    // Update stream device buffers and bindings
    vertex_stream.update_device_buffer(device, queue);
    index_stream.update_device_buffer(device, queue);
    uniform_stream.update_device_buffer(device, queue);

    // Order draw commands to minimize state changes
    std::sort(begin(draw_cmds), end(draw_cmds), [](DrawCommand const& a, DrawCommand const& b) {
        if (a.pipeline != b.pipeline)
            return a.pipeline < b.pipeline;
        else if (a.material_bindings != b.material_bindings)
            return a.material_bindings < b.material_bindings;
        else
            return a.geometry_bindings < b.geometry_bindings;
    });

    // Submit draw commands
    {
        WGPURenderPipeline prev_pipeline{};
        WGPUBindGroup prev_material_bg{};
        WGPUBindGroup prev_geometry_bg{};
        WGPUBuffer prev_index_buf{};

        if (pass.bindings)
            wgpuRenderPassEncoderSetBindGroup(
                encoder,
                u32(BindSlot::Pass),
                pass.bindings,
                0,
                nullptr);

        for (auto const& cmd : draw_cmds)
        {
            if (cmd.pipeline != prev_pipeline)
            {
                wgpuRenderPassEncoderSetPipeline(encoder, cmd.pipeline);
                prev_pipeline = cmd.pipeline;
            }

            if (cmd.material_bindings && cmd.material_bindings != prev_material_bg)
            {
                wgpuRenderPassEncoderSetBindGroup(
                    encoder,
                    u32(BindSlot::Material),
                    cmd.material_bindings,
                    0,
                    nullptr);

                prev_material_bg = cmd.material_bindings;
            }

            if (cmd.flags & DrawCommand::Flags_UseProceduralGeometry)
            {
                // Skip binding update if geometry is generated procedurally within the shader
                // ...
            }
            else
            {
                WGPUBindGroup const geometry_bg = cmd.geometry_bindings //
                    ? cmd.geometry_bindings
                    : vertex_stream.bindings();

                if (geometry_bg && geometry_bg != prev_geometry_bg)
                {
                    wgpuRenderPassEncoderSetBindGroup(
                        encoder,
                        u32(BindSlot::Geometry),
                        geometry_bg,
                        0,
                        nullptr);

                    prev_geometry_bg = geometry_bg;
                }
            }

            bool const needs_index_buf = cmd.type == DrawCommand::Type::DrawIndexed
                || cmd.type == DrawCommand::Type::DrawIndexedIndirect;

            if (needs_index_buf)
            {
                WGPUBuffer index_buf{};
                WGPUIndexFormat index_fmt{};
                if (cmd.index_buffer)
                {
                    index_buf = cmd.index_buffer;
                    index_fmt = cmd.index_format;
                }
                else
                {
                    index_buf = index_stream.device_buffer();
                    index_fmt = index_stream.format;
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

            u32 const uniform_offsets[]{
                pass_uniform_offset,
                cmd.uniform_offsets.material,
                cmd.uniform_offsets.geometry,
                cmd.uniform_offsets.object,
            };

            wgpuRenderPassEncoderSetBindGroup(
                encoder,
                u32(BindSlot::Uniform),
                uniform_stream.bindings(),
                size(uniform_offsets),
                uniform_offsets);

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
    vertex_stream.clear();
    index_stream.clear();
    uniform_stream.clear();
}

} // namespace wgpu::sandbox
