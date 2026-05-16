#pragma once

#include <webgpu/webgpu.h>

#include <dr/dynamic_array.hpp>
#include <dr/hash.hpp>
#include <dr/hash_map.hpp>
#include <dr/span.hpp>

#include <wgpu_default_limits.hpp>

#include "basic_types.hpp"
#include "gpu_resource.hpp"

namespace wgpu::sandbox
{

struct BufferStage
{
    DynamicArray<u8> host_buf;
    GpuBuffer device_buf{};
    usize device_size{};

    u32 append(Span<u8 const> const& bytes, usize align);
    bool update_device(WGPUDevice device, WGPUQueue queue, WGPUBufferUsage usage);
};

// TODO(dr): Add separate UniformStream type

struct GeometryStream
{
    static constexpr WGPUIndexFormat index_format{WGPUIndexFormat_Uint32};
    static constexpr u32 num_vertex_slots = default_max_dynamic_storage_buffers_per_pipeline_layout;

    static void init_shared_resources(WGPUDevice device);

    static WGPUBindGroupLayout vertex_bindings_layout();

    u32 push_vertices(Span<u8 const> const& data);

    template <u8 slot>
    u32 push_vertices_once(void const* key, Span<u8 const> const& data)
    {
        static_assert(slot < num_vertex_slots);
        return push_vertices_once({key, slot}, data);
    }

    u32 push_indices(Span<u8 const> const& data);
    u32 push_indices_once(void const* key, Span<u8 const> const& data);

    void update_device_buffers(WGPUDevice device, WGPUQueue queue);

    WGPUBindGroup vertex_bindings() const { return vertex_bg_; }

    WGPUBuffer index_buffer() const { return index_stage_.device_buf; }

    void clear();

  private:
    struct VertexKey
    {
        void const* src;
        u8 slot;
        bool operator==(VertexKey const& other) const;

        struct Hash : HighQualityHash
        {
            usize operator()(VertexKey const& key) const;
        };
    };

    BufferStage vertex_stage_;
    HashMap<VertexKey, u32, VertexKey::Hash> vertex_offsets_;
    GpuBindGroup vertex_bg_{};
    u32 last_vertex_offset_{};
    u32 max_vertex_size_{};
    u32 bound_vertex_size_{};

    BufferStage index_stage_;
    HashMap<void const*, u32> index_offsets_;

    u32 push_vertices_once(VertexKey const& key, Span<u8 const> const& data);

    void rebuild_bindings(WGPUDevice device);
};

} // namespace wgpu::sandbox
