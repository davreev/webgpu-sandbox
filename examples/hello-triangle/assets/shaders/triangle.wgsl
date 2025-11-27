struct TriVertex {
    position : vec2f,
    color: vec3f
};

const tri_verts = array(
    TriVertex( vec2f( -0.5, -0.5), vec3f(1.0, 0.2, 0.2)),
    TriVertex( vec2f( 0.5, -0.5), vec3f(0.2, 1.0, 0.2)),
    TriVertex( vec2f( 0.0, 0.5 ), vec3f(0.2, 0.2, 1.0)),
);

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f
};

@vertex
fn vs_main(@builtin(vertex_index) vi: u32) -> VertexOut {
    let v = tri_verts[vi];
    return VertexOut(vec4f(v.position, 0.0, 1.0), v.color);
}

struct FragmentIn {
    @location(0) color: vec3f
};

@fragment
fn fs_main(in : FragmentIn) -> @location(0) vec4f {
    return vec4f(in.color, 1.0);
}