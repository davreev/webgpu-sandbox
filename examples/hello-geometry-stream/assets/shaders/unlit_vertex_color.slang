struct ObjectData {
    float4x4 world_to_clip;
    uint vertex_offset;
    uint instance_offset;
};

layout(binding = 0, set = 2) StructuredBuffer<float4> vertex_positions;
layout(binding = 1, set = 2) StructuredBuffer<float4> instance_data;

layout(binding = 3, set = 3) ConstantBuffer<ObjectData> object;

struct VertexOut {
    float4 position : SV_Position;
    float3 color;
};

[shader("vertex")]
VertexOut vs_main(uint vi: SV_VertexID, uint ii: SV_InstanceID) {
    let p = vertex_positions[object.vertex_offset + vi].xyz;
    let inst = instance_data[object.instance_offset + ii];

    let center = inst.xyz;
    let scale = inst.w;
    let p_world = center + p * scale;

    VertexOut o;
    o.position = mul(object.world_to_clip, float4(p_world, 1.0));
    o.color = p * 0.25 + 0.5;
    return o;
}

[shader("fragment")]
float4 fs_main(float3 color) : SV_Target { return float4(color, 1.0); }
