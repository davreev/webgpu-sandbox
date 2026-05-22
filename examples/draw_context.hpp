#pragma once

#include <webgpu/webgpu.h>

#include <dr/container_utils.hpp>
#include <dr/dynamic_array.hpp>
#include <dr/hash_map.hpp>
#include <dr/span.hpp>

#include "basic_types.hpp"
#include "draw_command.hpp"
#include "draw_streams.hpp"

namespace wgpu::sandbox
{

struct DrawContext
{
    DynamicArray<DrawCommand> draw_cmds;
    VertexStream vertex_stream;
    IndexStream<u32> index_stream;
    UniformStream uniform_stream;

    struct PassInfo
    {
        WGPUBindGroup bindings;
        Span<u8 const> uniform_data;
    };

    void submit_draw_cmds(
        WGPUDevice device,
        WGPUQueue queue,
        WGPURenderPassEncoder encoder,
        PassInfo const& pass = {});
};

enum struct BindSlot : u8
{
    Pass = 0,
    Material,
    Geometry,
    Uniform,
};

} // namespace wgpu::sandbox
