#include "draw_streams.hpp"

#include <cassert>

#include <dr/memory.hpp>

#include <wgpu_default_limits.hpp>

namespace wgpu::sandbox
{
namespace
{

constexpr u32 uniform_buffer_alignment = default_min_uniform_buffer_offset_alignment;

GpuBindGroupLayout vertex_stream_bgl{};
GpuBindGroupLayout uniform_stream_bgl{};

void vertex_stream_check_device_limits(WGPUDevice const device)
{
    WGPULimits limits{};
    [[maybe_unused]]
    auto const status = wgpuDeviceGetLimits(device, &limits);
    assert(status == WGPUStatus::WGPUStatus_Success);
    assert(VertexStream::num_lanes <= limits.maxStorageBuffersPerShaderStage);
}

void uniform_stream_check_device_limits(WGPUDevice const device)
{
    WGPULimits limits{};
    [[maybe_unused]]
    auto const status = wgpuDeviceGetLimits(device, &limits);
    assert(status == WGPUStatus::WGPUStatus_Success);
    assert(UniformStream::num_lanes <= limits.maxDynamicUniformBuffersPerPipelineLayout);
    assert(uniform_buffer_alignment >= limits.minUniformBufferOffsetAlignment);
}

constexpr usize aligned_size(usize const size, usize const align)
{
    return (size + align - 1) & ~(align - 1);
}

} // namespace

//
// VertexStream
//

void VertexStream::init_bindings_layout(WGPUDevice const device)
{
    vertex_stream_check_device_limits(device);

    WGPUBindGroupLayoutEntry entries[num_lanes]{};
    for (u32 i = 0; i < num_lanes; ++i)
    {
        entries[i] = {
            .binding = i,
            .visibility = WGPUShaderStage_Vertex,
            .buffer{
                .type = WGPUBufferBindingType_ReadOnlyStorage,
                .hasDynamicOffset = false,
                .minBindingSize = 0,
            },
        };
    }
    WGPUBindGroupLayoutDescriptor const desc{
        .entryCount = num_lanes,
        .entries = entries,
    };
    vertex_stream_bgl = wgpuDeviceCreateBindGroupLayout(device, &desc);
}

WGPUBindGroupLayout VertexStream::bindings_layout()
{
    assert(vertex_stream_bgl);
    return vertex_stream_bgl;
}

u32 VertexStream::push(Span<u8 const> const& bytes, usize const align)
{
    return stage_.append(bytes, align);
}

u32 VertexStream::push_once(LaneKey const& key, Span<u8 const> const& bytes, usize const align)
{
    auto const [it, ok] = offsets_.try_emplace(key);
    if (ok)
        it->second = push(bytes, align);

    return it->second;
}

void VertexStream::rebuild_bindings(WGPUDevice const device)
{
    assert(vertex_stream_bgl);

    WGPUBindGroupEntry entries[num_lanes]{};
    for (u32 i = 0; i < num_lanes; ++i)
    {
        entries[i] = {
            .binding = i,
            .buffer = stage_.device_buf,
            .size = WGPU_WHOLE_SIZE,
        };
    }
    WGPUBindGroupDescriptor const desc{
        .layout = vertex_stream_bgl,
        .entryCount = num_lanes,
        .entries = entries,
    };
    bindings_ = wgpuDeviceCreateBindGroup(device, &desc);
}

void VertexStream::update_device_buffer(WGPUDevice const device, WGPUQueue const queue)
{
    if (!stage_.host_buf.empty())
    {
        // Rebuild bindings if the vertex buffer was reallocated
        bool bindings_dirty = stage_.update_device(device, queue, WGPUBufferUsage_Storage);

        if (bindings_dirty)
            rebuild_bindings(device);
    }
}

void VertexStream::clear()
{
    stage_.host_buf.clear();
    offsets_.clear();
}

//
// IndexStream
//

template <typename Index>
u32 IndexStream<Index>::push(Span<Index const> const& indices)
{
    return stage_.append(as<u8>(indices), 1) / sizeof(Index);
}

template <typename Index>
u32 IndexStream<Index>::push_once(void const* key, Span<Index const> const& indices)
{
    auto const [it, ok] = offsets_.try_emplace(key);
    if (ok)
        it->second = push(indices);

    return it->second;
}

template <typename Index>
void IndexStream<Index>::update_device_buffer(WGPUDevice const device, WGPUQueue const queue)
{
    stage_.update_device(device, queue, WGPUBufferUsage_Index);
}

template <typename Index>
void IndexStream<Index>::clear()
{
    stage_.host_buf.clear();
    offsets_.clear();
}

template struct IndexStream<u32>;
template struct IndexStream<u16>;

//
// UniformStream
//

void UniformStream::init_bindings_layout(WGPUDevice const device)
{
    uniform_stream_check_device_limits(device);

    WGPUBindGroupLayoutEntry entries[num_lanes]{};
    for (u8 i = 0; i < num_lanes; ++i)
    {
        entries[i] = {
            .binding = i,
            .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .buffer{
                .type = WGPUBufferBindingType_Uniform,
                .hasDynamicOffset = true,
                .minBindingSize = 0,
            },
        };
    }
    WGPUBindGroupLayoutDescriptor const desc{
        .entryCount = num_lanes,
        .entries = entries,
    };
    uniform_stream_bgl = wgpuDeviceCreateBindGroupLayout(device, &desc);
}

WGPUBindGroupLayout UniformStream::bindings_layout()
{
    assert(uniform_stream_bgl);
    return uniform_stream_bgl;
}

u32 UniformStream::push(Span<u8 const> const& bytes)
{
    // Track the size of the largest chunk to correctly size bind group entries
    if (bytes.size() > max_size_)
        max_size_ = bytes.size();

    // Also track the last offset to determine necessary host buffer padding
    last_offset_ = stage_.append(bytes, uniform_buffer_alignment);
    return last_offset_;
}

u32 UniformStream::push_once(LaneKey const& key, Span<u8 const> const& bytes)
{
    auto const [it, ok] = offsets_.try_emplace(key);
    if (ok)
        it->second = push(bytes);

    return it->second;
}

void UniformStream::rebuild_bindings(WGPUDevice const device)
{
    assert(uniform_stream_bgl);
    assert(max_size_ > 0);

    WGPUBindGroupEntry entries[num_lanes]{};
    for (u8 i = 0; i < num_lanes; ++i)
    {
        entries[i] = {
            .binding = i,
            .buffer = stage_.device_buf,
            .size = max_size_,
        };
    }
    WGPUBindGroupDescriptor const desc{
        .layout = uniform_stream_bgl,
        .entryCount = num_lanes,
        .entries = entries,
    };
    bindings_ = wgpuDeviceCreateBindGroup(device, &desc);
    bound_size_ = max_size_;
}

void UniformStream::update_device_buffer(WGPUDevice const device, WGPUQueue const queue)
{
    // Early out if nothing to update
    if (stage_.host_buf.empty())
        return;

    // Pad the host buffer to account for largest binding view
    stage_.host_buf.resize(last_offset_ + max_size_);

    // Rebuild bindings if the buffer was reallocated or its bind size has changed
    bool bindings_dirty = stage_.update_device(device, queue, WGPUBufferUsage_Uniform);
    bindings_dirty |= (max_size_ != bound_size_);

    if (bindings_dirty)
        rebuild_bindings(device);
}

void UniformStream::clear()
{
    stage_.host_buf.clear();
    offsets_.clear();
    max_size_ = 0;
    last_offset_ = 0;
}

//
// BufferStage
//

u32 BufferStage::append(Span<u8 const> const& bytes, usize const align)
{
    usize const offset = aligned_size(host_buf.size(), align);
    host_buf.resize(offset);
    host_buf.insert(host_buf.end(), begin(bytes), end(bytes));
    return offset;
}

bool BufferStage::update_device(
    WGPUDevice const device,
    WGPUQueue const queue,
    WGPUBufferUsage const usage)
{
    if (host_buf.empty())
        return false;

    bool resized = false;
    if (host_buf.size() > device_size)
    {
        device_size = host_buf.capacity();
        WGPUBufferDescriptor const desc{
            .usage = usage | WGPUBufferUsage_CopyDst,
            .size = device_size,
        };
        device_buf = wgpuDeviceCreateBuffer(device, &desc);
        resized = true;
    }

    wgpuQueueWriteBuffer(queue, device_buf, 0, host_buf.data(), host_buf.size());
    return resized;
}

//
// LaneKey
//

bool LaneKey::operator==(LaneKey const& other) const
{
    return src == other.src && lane == other.lane;
}

usize LaneKey::Hash::operator()(LaneKey const& key) const
{
    return hash_mix(usize(key.src), usize(key.lane));
}

} // namespace wgpu::sandbox
