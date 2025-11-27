Sampler2D color_sampler;

struct Uniforms
{
    float4x4 local_to_clip;
}
ConstantBuffer<Uniforms> uniforms;

struct VertexIn {
    float3 position;
    float2 tex_coords;
}

struct VertexOut {
    float4 position : SV_Position;
    float2 tex_coords;
    float3 db_color;
}

[shader("vertex")]
VertexOut vs_main(VertexIn v_in) {
    VertexOut v_out;
    v_out.position = mul(uniforms.local_to_clip, float4(v_in.position, 1.0));
    v_out.tex_coords = v_in.tex_coords;
    v_out.db_color = v_in.position.xyz;
    return v_out;
}

[shader("fragment")]
float4 fs_main(float2 tex_coords, float3 db_color) : SV_Target{
    // return float4(db_color, 1.0); // DEBUG(dr)
    return color_sampler.SampleLevel(tex_coords, 0.0);
}
