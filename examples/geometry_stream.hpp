#pragma once

#include <webgpu/webgpu.h>

#include <dr/basic_types.hpp>
#include <dr/dynamic_array.hpp>
#include <dr/hash.hpp>
#include <dr/hash_map.hpp>
#include <dr/span.hpp>

#include "gpu_resource.hpp"

#include "dr_shim.hpp"

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

struct GeometryStream
{
    static void init_shared_resources(WGPUDevice device);

    u32 push_vertices(Span<u8 const> const& data);
    u32 push_vertices_once(void const* key, u8 slot, Span<u8 const> const& data);

    u32 push_indices(Span<u8 const> const& data);
    u32 push_indices_once(void const* key, Span<u8 const> const& data);

    void update_device_buffers(WGPUDevice device, WGPUQueue queue);

    WGPUBindGroup bindings() const { return bindings_; }

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
    BufferStage index_stage_;
    HashMap<VertexKey, u32, VertexKey::Hash> vertex_offsets_;
    HashMap<void const*, u32> index_offsets_;
    GpuBindGroup bindings_{};

    void rebuild_bindings(WGPUDevice device);
};

} // namespace wgpu::sandbox
