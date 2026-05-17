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

struct VertexStream
{
    static constexpr u32 num_slots = default_max_dynamic_storage_buffers_per_pipeline_layout;

    static void init_shared_resources(WGPUDevice device);

    static WGPUBindGroupLayout bindings_layout();

    u32 push(Span<u8 const> const& bytes);

    template <u8 slot>
    u32 push_once(void const* key, Span<u8 const> const& bytes)
    {
        static_assert(slot < num_slots);
        return push_once({key, slot}, bytes);
    }

    void update_device_buffer(WGPUDevice device, WGPUQueue queue);

    WGPUBuffer device_buffer() const { return stage_.device_buf; }

    WGPUBindGroup bindings() const { return bindings_; }

    void clear();

  private:
    struct Key
    {
        void const* src;
        u8 slot;
        bool operator==(Key const& other) const;

        struct Hash : HighQualityHash
        {
            usize operator()(Key const& key) const;
        };
    };

    BufferStage stage_;
    HashMap<Key, u32, Key::Hash> offsets_;
    GpuBindGroup bindings_{};
    u32 last_offset_{};
    u32 max_size_{};
    u32 bound_size_{};

    u32 push_once(Key const& key, Span<u8 const> const& bytes);

    void rebuild_bindings(WGPUDevice device);
};

template <typename Index>
struct IndexStream
{
    // TODO(dr): Infer this from the index type
    static constexpr WGPUIndexFormat format{WGPUIndexFormat_Uint32};

    u32 push(Span<Index const> const& indices);
    u32 push_once(void const* key, Span<Index const> const& indices);

    void update_device_buffer(WGPUDevice device, WGPUQueue queue);

    WGPUBuffer device_buffer() const { return index_stage_.device_buf; }

    void clear();

  private:
    BufferStage index_stage_;
    HashMap<void const*, u32> index_offsets_;
};

// TODO(dr): Add separate UniformStream type

} // namespace wgpu::sandbox
