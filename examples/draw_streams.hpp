#pragma once

#include <webgpu/webgpu.h>

#include <dr/dynamic_array.hpp>
#include <dr/hash.hpp>
#include <dr/hash_map.hpp>
#include <dr/memory.hpp>
#include <dr/span.hpp>

#include <wgpu_default_limits.hpp>

#include "basic_types.hpp"
#include "gpu_resource.hpp"
#include "traits_fwd.hpp"

// TODO(dr): Rename file "draw_streams.hpp"

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

struct LaneKey
{
    void const* src;
    u8 lane;

    bool operator==(LaneKey const& other) const;

    struct Hash : HighQualityHash
    {
        usize operator()(LaneKey const& key) const;
    };
};

struct VertexStream
{
    static constexpr u8 num_lanes = 4;

    static void init_bindings_layout(WGPUDevice device);

    static WGPUBindGroupLayout bindings_layout();

    template <typename T>
    u32 push(Span<T const> const& items)
    {
        return push(as<u8>(items), sizeof(T)) / sizeof(T);
    }

    template <u8 lane, typename T>
    u32 push_once(void const* key, Span<T const> const& items)
    {
        return push_once<lane>(key, as<u8>(items), sizeof(T)) / sizeof(T);
    }

    void update_device_buffer(WGPUDevice device, WGPUQueue queue);

    WGPUBuffer device_buffer() const { return stage_.device_buf; }

    WGPUBindGroup bindings() const { return bindings_; }

    void clear();

  private:
    BufferStage stage_;
    HashMap<LaneKey, u32, LaneKey::Hash> offsets_;
    GpuBindGroup bindings_{};

    u32 push(Span<u8 const> const& bytes, usize align);

    template <u8 lane>
    u32 push_once(void const* key, Span<u8 const> const& bytes, usize align)
    {
        static_assert(lane < num_lanes);
        return push_once({key, lane}, bytes, align);
    }

    u32 push_once(LaneKey const& key, Span<u8 const> const& bytes, usize align);

    void rebuild_bindings(WGPUDevice device);
};

template <typename Index>
struct IndexStream
{
    static constexpr WGPUIndexFormat format = Traits<IndexStream>::format;

    u32 push(Span<Index const> const& indices);
    u32 push_once(void const* key, Span<Index const> const& indices);

    void update_device_buffer(WGPUDevice device, WGPUQueue queue);

    WGPUBuffer device_buffer() const { return stage_.device_buf; }

    void clear();

  private:
    BufferStage stage_;
    HashMap<void const*, u32> offsets_;
};

template <>
struct Traits<IndexStream<u32>>
{
    static constexpr WGPUIndexFormat format = WGPUIndexFormat_Uint32;
};

template <>
struct Traits<IndexStream<u16>>
{
    static constexpr WGPUIndexFormat format = WGPUIndexFormat_Uint16;
};

struct UniformStream
{
    static constexpr u8 num_lanes = 4;

    static void init_bindings_layout(WGPUDevice device);

    static WGPUBindGroupLayout bindings_layout();

    u32 push(Span<u8 const> const& bytes);

    template <u8 lane>
    u32 push_once(void const* key, Span<u8 const> const& bytes)
    {
        static_assert(lane < num_lanes);
        return push_once({key, lane}, bytes);
    }

    void update_device_buffer(WGPUDevice device, WGPUQueue queue);

    WGPUBuffer device_buffer() const { return stage_.device_buf; }

    WGPUBindGroup bindings() const { return bindings_; }

    void clear();

  private:
    BufferStage stage_;
    HashMap<LaneKey, u32, LaneKey::Hash> offsets_;
    GpuBindGroup bindings_{};
    u32 last_offset_{};
    u32 max_size_{};
    u32 bound_size_{};

    u32 push_once(LaneKey const& key, Span<u8 const> const& bytes);

    void rebuild_bindings(WGPUDevice device);
};

} // namespace wgpu::sandbox
