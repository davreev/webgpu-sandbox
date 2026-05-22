#pragma once

#include <webgpu/webgpu.h>

#include <dr/result.hpp>

#include <wgpu_default_limits.hpp>

#include "basic_types.hpp"

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

    enum Flags : u32
    {
        Flags_None = 0,
        Flags_UseProceduralGeometry = 1 << 0,
        // ...
        Flags_All = ~Flags_None,
    };

    WGPURenderPipeline pipeline{};
    WGPUBindGroup material_bindings{};
    WGPUBindGroup geometry_bindings{};
    WGPUBuffer index_buffer{};
    WGPUIndexFormat index_format{};
    struct
    {
        u32 material{};
        u32 geometry{};
        u32 object{};
    } uniform_offsets;
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
    Flags flags{};
};

} // namespace wgpu::sandbox
