#pragma once

namespace wgpu::sandbox
{

constexpr char const* shader_src = R"(
struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) tex_coord: vec2f
};

@vertex
fn vs_main(@builtin(vertex_index) v: u32) -> VertexOut {
    var xy = array<vec2f, 3>(
        vec2f(-0.5, -0.5),
        vec2f(0.5, -0.5),
        vec2f(0.0, 0.5)
    );
    var uv = array<vec2f, 3>(
        vec2f(1.0, 0.0),
        vec2f(0.0, 1.0),
        vec2f(0.0, 0.0)
    );
    return VertexOut(vec4f(xy[v], 0.0, 1.0), uv[v]);
}

struct FragmentIn {
    @location(0) tex_coord: vec2f
};

@fragment
fn fs_main(in : FragmentIn) -> @location(0) vec4f {
    let u = in.tex_coord.x;
    let v = in.tex_coord.y;
    let w = 1.0 - u - v;
    let col = vec3f(1.0, 0.2, 0.2) * u + vec3f(0.2, 1.0, 0.2) * v + vec3f(0.2, 0.2, 1.0) * w;
    return vec4f(col, 1.0);
}
)";

} // namespace wgpu::sandbox