#include "geometry_stream.hpp"

#include <cassert>

#include <dr/memory.hpp>

#include <wgpu_default_limits.hpp>

namespace wgpu::sandbox
{
namespace
{

constexpr u32 vertex_buffer_alignment = default_min_storage_buffer_offset_alignment;

GpuBindGroupLayout vertex_bgl{};

constexpr usize aligned_size(usize const size, usize const align)
{
    return (size + align - 1) & ~(align - 1);
}

void check_vertex_device_limits(WGPUDevice const device)
{
    WGPULimits limits{};
    [[maybe_unused]]
    auto const status = wgpuDeviceGetLimits(device, &limits);
    assert(status == WGPUStatus::WGPUStatus_Success);
    assert(VertexStream::num_slots <= limits.maxDynamicStorageBuffersPerPipelineLayout);
    assert(vertex_buffer_alignment >= limits.minStorageBufferOffsetAlignment);
}

} // namespace

//
// VertexStream
//

void VertexStream::init_shared_resources(WGPUDevice const device)
{
    check_vertex_device_limits(device);

    WGPUBindGroupLayoutEntry entries[num_slots]{};
    for (u32 i = 0; i < num_slots; ++i)
    {
        entries[i] = {
            .binding = i,
            .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .buffer{
                .type = WGPUBufferBindingType_ReadOnlyStorage,
                .hasDynamicOffset = true,
                .minBindingSize = 0,
            },
        };
    }
    WGPUBindGroupLayoutDescriptor const desc{
        .entryCount = num_slots,
        .entries = entries,
    };
    vertex_bgl = wgpuDeviceCreateBindGroupLayout(device, &desc);
}

WGPUBindGroupLayout VertexStream::bindings_layout()
{
    assert(vertex_bgl);
    return vertex_bgl;
}

bool VertexStream::Key::operator==(Key const& other) const
{
    return src == other.src && slot == other.slot;
}

usize VertexStream::Key::Hash::operator()(Key const& key) const
{
    return hash_mix(usize(key.src), usize(key.slot));
}

u32 VertexStream::push(Span<u8 const> const& bytes)
{
    // Track the size of the largest chunk to correctly size bind group entries
    if (bytes.size() > max_size_)
        max_size_ = bytes.size();

    // Also track the last offset to determine necessary host buffer padding
    last_offset_ = stage_.append(bytes, vertex_buffer_alignment);
    return last_offset_;
}

u32 VertexStream::push_once(Key const& key, Span<u8 const> const& bytes)
{
    auto const [it, ok] = offsets_.try_emplace(key);
    if (ok)
        it->second = push(bytes);

    return it->second;
}

void VertexStream::rebuild_bindings(WGPUDevice const device)
{
    assert(vertex_bgl);
    assert(max_size_ > 0);

    // TODO(dr): If we're relying on vertex pulling when using geometry streams, could
    // alternatively skip dynamic offsets and just provide storage buffer offsets as uniforms

    WGPUBindGroupEntry entries[num_slots]{};
    for (u32 i = 0; i < num_slots; ++i)
    {
        entries[i] = {
            .binding = i,
            .buffer = stage_.device_buf,
            .size = max_size_,
        };
    }
    WGPUBindGroupDescriptor const desc{
        .layout = vertex_bgl,
        .entryCount = num_slots,
        .entries = entries,
    };
    bindings_ = wgpuDeviceCreateBindGroup(device, &desc);
    bound_size_ = max_size_;
}

void VertexStream::update_device_buffer(WGPUDevice const device, WGPUQueue const queue)
{
    if (!stage_.host_buf.empty())
    {
        // Pad the host buffer to account for the largest binding view when using dynamic offsets
        stage_.host_buf.resize(last_offset_ + max_size_);

        // Rebuild bindings if the vertex buffer was reallocated or its bind size has changed
        bool bindings_dirty = stage_.update_device(device, queue, WGPUBufferUsage_Storage);
        bindings_dirty |= (max_size_ != bound_size_);

        if (bindings_dirty)
            rebuild_bindings(device);
    }
}

void VertexStream::clear()
{
    stage_.host_buf.clear();
    offsets_.clear();
    max_size_ = 0;
    last_offset_ = 0;
}

//
// IndexStream
//

template <typename Index>
u32 IndexStream<Index>::push(Span<Index const> const& indices)
{
    return index_stage_.append(as<u8>(indices), 1) / sizeof(Index);
}

template <typename Index>
u32 IndexStream<Index>::push_once(void const* key, Span<Index const> const& indices)
{
    auto const [it, ok] = index_offsets_.try_emplace(key);
    if (ok)
        it->second = push(indices);

    return it->second;
}

template <typename Index>
void IndexStream<Index>::update_device_buffer(WGPUDevice const device, WGPUQueue const queue)
{
    index_stage_.update_device(device, queue, WGPUBufferUsage_Index);
}

template <typename Index>
void IndexStream<Index>::clear()
{
    index_stage_.host_buf.clear();
    index_offsets_.clear();
}

template struct IndexStream<u32>;
template struct IndexStream<i32>;

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

} // namespace wgpu::sandbox
