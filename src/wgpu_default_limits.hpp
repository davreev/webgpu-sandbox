#pragma once

#include <cstdint>

namespace wgpu::sandbox
{

// Default limits from the WebGPU spec: https://gpuweb.github.io/gpuweb/#limits

// Textures
constexpr uint32_t default_max_texture_dimension_1d = 8192;
constexpr uint32_t default_max_texture_dimension_2d = 8192;
constexpr uint32_t default_max_texture_dimension_3d = 2048;
constexpr uint32_t default_max_texture_array_layers = 256;

// Bind groups and bindings
constexpr uint32_t default_max_bind_groups = 4;
constexpr uint32_t default_max_bind_groups_plus_vertex_buffers = 24;
constexpr uint32_t default_max_bindings_per_bind_group = 1000;
constexpr uint32_t default_max_dynamic_uniform_buffers_per_pipeline_layout = 8;
constexpr uint32_t default_max_dynamic_storage_buffers_per_pipeline_layout = 4;
constexpr uint32_t default_max_sampled_textures_per_shader_stage = 16;
constexpr uint32_t default_max_samplers_per_shader_stage = 16;
constexpr uint32_t default_max_storage_buffers_per_shader_stage = 8;
constexpr uint32_t default_max_storage_textures_per_shader_stage = 4;
constexpr uint32_t default_max_uniform_buffers_per_shader_stage = 12;

// Buffer sizes and alignment
constexpr uint64_t default_max_uniform_buffer_binding_size = 65536;
constexpr uint64_t default_max_storage_buffer_binding_size = 134217728; // 128 MiB
constexpr uint32_t default_min_uniform_buffer_offset_alignment = 256;
constexpr uint32_t default_min_storage_buffer_offset_alignment = 256;
constexpr uint64_t default_max_buffer_size = 4294967295; // 4 GiB - 1

// Vertex
constexpr uint32_t default_max_vertex_buffers = 16;
constexpr uint32_t default_max_vertex_attributes = 32;
constexpr uint32_t default_max_vertex_buffer_array_stride = 2048;

// Inter-stage and attachments
constexpr uint32_t default_max_inter_stage_shader_variables = 16;
constexpr uint32_t default_max_color_attachments = 8;
constexpr uint32_t default_max_color_attachment_bytes_per_sample = 32;

// Compute
constexpr uint32_t default_max_compute_workgroup_storage_size = 49152; // 48 KiB
constexpr uint32_t default_max_compute_invocations_per_workgroup = 256;
constexpr uint32_t default_max_compute_workgroup_size_x = 256;
constexpr uint32_t default_max_compute_workgroup_size_y = 256;
constexpr uint32_t default_max_compute_workgroup_size_z = 64;
constexpr uint32_t default_max_compute_workgroups_per_dimension = 65535;

} // namespace wgpu::sandbox
