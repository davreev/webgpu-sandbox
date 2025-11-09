struct TriVertex {
    float2 position;
    float3 color;
};

static const TriVertex tri_verts[] = {
    { { -0.5, -0.5 }, { 1.0, 0.2, 0.2 } },
    { { 0.5, -0.5 }, { 0.2, 1.0, 0.2 } },
    { { 0.0, 0.5 }, { 0.2, 0.2, 1.0 } },
};

struct VertexOut {
    float4 position : SV_Position;
    float3 color;
};

[shader("vertex")]
VertexOut vs_main(int vi: SV_VertexID) {
    let v = tri_verts[vi];
    return { { v.position, 0.0, 1.0 }, { v.color } };
}

[shader("fragment")]
float4 fs_main(float3 color) : SV_Target { return float4(color, 1.0); }
