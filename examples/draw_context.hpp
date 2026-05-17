#pragma once

#include <webgpu/webgpu.h>

#include <dr/container_utils.hpp>
#include <dr/dynamic_array.hpp>
#include <dr/hash_map.hpp>
#include <dr/span.hpp>

#include "basic_types.hpp"
#include "draw_command.hpp"
#include "geometry_stream.hpp"
#include "gpu_resource.hpp"

namespace wgpu::sandbox
{

struct DrawContext
{
    static void init_shared_resources(WGPUDevice device);

    DynamicArray<DrawCommand> draw_cmds;
    struct {
        VertexStream vertex;
        IndexStream<i32> index;
    } streams;

    u32 push_uniforms(Span<u8 const> const& data);
    u32 push_uniforms_once(void const* key, Span<u8 const> const& data);

    void submit_draw_cmds(
        WGPUDevice device,
        WGPUQueue queue,
        WGPURenderPassEncoder encoder,
        WGPUBindGroup pass_bg = {});

  private:
    BufferStage uniform_stage_;
    HashMap<void const*, u32> uniform_offsets_;
    GpuBindGroup uniform_bg_{};

    void rebuild_uniform_bg(WGPUDevice device);
};

enum struct BindSlot : u8
{
    Pass = 0,
    Material,
    Geometry,
    Object,
};

} // namespace wgpu::sandbox
