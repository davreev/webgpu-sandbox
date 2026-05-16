struct ObjectData {
    float4x4 world_to_clip;
};

// NOTE(dr): By convention, geometry resources are bound to group 2 and per-object data is bound
// to group 3

layout(binding = 0, set = 2) StructuredBuffer<float4> vertex_positions;
layout(binding = 1, set = 2) StructuredBuffer<float4> instance_data;

layout(binding = 0, set = 3) ConstantBuffer<ObjectData> object;

struct VertexOut {
    float4 position : SV_Position;
    float3 color;
};

[shader("vertex")]
VertexOut vs_main(uint vi: SV_VertexID, uint ii: SV_InstanceID) {
    let vp = vertex_positions[vi].xyz;
    let inst = instance_data[ii];
    let center = inst.xyz;
    let scale = inst.w;
    let world_pos = center + vp * scale;

    VertexOut o;
    o.position = mul(object.world_to_clip, float4(world_pos, 1.0));
    // Derive color from vertex position. Vertices are in [-2, 2].
    o.color = vp * 0.25 + 0.5;
    return o;
}

[shader("fragment")]
float4 fs_main(float3 color) : SV_Target { return float4(color, 1.0); }
