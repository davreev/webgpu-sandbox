#pragma once

namespace wgpu::sandbox
{

constexpr char const* shader_src = R"(
@group(0) @binding(0) var<storage, read_write> vals: array<f32>;

@compute @workgroup_size(32, 1, 1)
fn compute_main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let i = global_id.x;

    if(i >= arrayLength(&vals)) {
        return;
    }

    vals[i] = f32(i);
}
)";

} // namespace wgpu::sandbox