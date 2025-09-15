@group(0) @binding(0) 
var color_texture: texture_2d<f32>;

@group(0) @binding(1) 
var color_sampler: sampler;

struct Uniforms {
    local_to_clip : mat4x4<f32>,
};

@group(0) @binding(2)
var<uniform> uniforms : Uniforms;

struct VertexIn {
    @location(0) position: vec3f,
    @location(1) tex_coords: vec2f,
};

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) tex_coords: vec2f,
    @location(1) db_color: vec3f,
};

@vertex
fn vs_main(in : VertexIn) -> VertexOut {
    var out : VertexOut;
    out.position = uniforms.local_to_clip * vec4f(in.position, 1.0);
    out.tex_coords = in.tex_coords;
    out.db_color = in.position.xyz;
    return out;
}

struct FragmentIn {
    @location(0) tex_coords: vec2f,
    @location(1) db_color: vec3f,
};

@fragment
fn fs_main(in : FragmentIn) -> @location(0) vec4f {
    // return vec4<f32>(in.db_color, 1.0); // DEBUG(dr)
    return textureSample(color_texture, color_sampler, in.tex_coords);
}
