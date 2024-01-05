#pragma once

namespace wgpu::hello_mesh
{

constexpr char const* shader_src = R"(
struct VertexIn {
    @location(0) position: vec2f,
    @location(1) tex_coord: vec2f
};

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) tex_coord: vec2f
};

@vertex
fn vs_main(in : VertexIn) -> VertexOut {
    return VertexOut(vec4f(in.position, 0.0, 1.0), in.tex_coord);
}

struct FragmentIn {
    @location(0) tex_coord: vec2f
};

@fragment
fn fs_main(in : FragmentIn) -> @location(0) vec4f {
    return vec4f(in.tex_coord, 0.5, 1.0);
}
)";

}