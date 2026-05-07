#pragma once

#include <webgpu/webgpu.h>

#include <dr/basic_types.hpp>
#include <dr/result.hpp>

#include "dr_shim.hpp"

namespace wgpu::sandbox
{

struct DrawCommand
{
    enum struct Type : u8
    {
        Undefined = 0,
        Draw,
        DrawIndexed,
        DrawIndirect,
        DrawIndexedIndirect,
    };

    WGPURenderPipeline pipeline{};
    WGPUBindGroup material_bg{};
    WGPUBindGroup geometry_bg{};
    WGPUBuffer index_buf{};
    Maybe<u32[4]> geometry_offsets{};
    u32 uniform_offset{};

    union
    {
        struct
        {
            u32 vertex_count{};
            u32 instance_count{};
            u32 first_vertex{};
            u32 first_instance{};
        } draw;
        struct
        {
            u32 index_count{};
            u32 instance_count{};
            u32 first_index{};
            i32 base_vertex{};
            u32 first_instance{};
        } draw_indexed;
        struct
        {
            WGPUBuffer buffer{};
            usize offset{};
        } draw_indirect;
    };
    Type type{};
};

} // namespace wgpu::sandbox
