#include "geometry_stream.hpp"

#include <cassert>

#include <wgpu_default_limits.hpp>

namespace wgpu::sandbox
{
namespace
{

constexpr u32 num_vertex_slots = GeometryStream::num_vertex_slots;
constexpr u32 vertex_buffer_alignment = default_min_storage_buffer_offset_alignment;

GpuBindGroupLayout bindings_layout{};

constexpr usize aligned_size(usize const size, usize const align)
{
    return (size + align - 1) & ~(align - 1);
}

void init_bindings_layout(WGPUDevice const device)
{
    WGPUBindGroupLayoutEntry entries[num_vertex_slots]{};
    for (u32 i = 0; i < num_vertex_slots; ++i)
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
        .entryCount = num_vertex_slots,
        .entries = entries,
    };
    bindings_layout = wgpuDeviceCreateBindGroupLayout(device, &desc);
}

} // namespace

void check_device_limits(WGPUDevice const device)
{
    WGPULimits limits{};
    [[maybe_unused]]
    auto const status = wgpuDeviceGetLimits(device, &limits);
    assert(status == WGPUStatus::WGPUStatus_Success);
    assert(num_vertex_slots <= limits.maxDynamicStorageBuffersPerPipelineLayout);
    assert(vertex_buffer_alignment >= limits.minStorageBufferOffsetAlignment);
}

void GeometryStream::init_shared_resources(WGPUDevice const device)
{
    check_device_limits(device);
    init_bindings_layout(device);
}

WGPUBindGroupLayout GeometryStream::vertex_bindings_layout()
{
    assert(bindings_layout);
    return bindings_layout;
}

bool GeometryStream::VertexKey::operator==(VertexKey const& other) const
{
    return src == other.src && slot == other.slot;
}

usize GeometryStream::VertexKey::Hash::operator()(VertexKey const& key) const
{
    return hash_mix(usize(key.src), usize(key.slot));
}

u32 GeometryStream::push_vertices(Span<u8 const> const& data)
{
    // Track the size of the largest chunk to correctly size bind group entries
    if (data.size() > max_vertex_size_)
        max_vertex_size_ = data.size();

    // Also track the last offset to determine necessary host buffer padding
    last_vertex_offset_ = vertex_stage_.append(data, vertex_buffer_alignment);
    return last_vertex_offset_;
}

u32 GeometryStream::push_vertices_once(VertexKey const& key, Span<u8 const> const& data)
{
    auto const [it, ok] = vertex_offsets_.try_emplace(key);
    if (ok)
        it->second = push_vertices(data);

    return it->second;
}

u32 GeometryStream::push_indices(Span<u8 const> const& data)
{
    return index_stage_.append(data, 1);
}

u32 GeometryStream::push_indices_once(void const* key, Span<u8 const> const& data)
{
    auto const [it, ok] = index_offsets_.try_emplace(key);
    if (ok)
        it->second = push_indices(data);

    return it->second;
}

void GeometryStream::rebuild_bindings(WGPUDevice const device)
{
    assert(bindings_layout);
    assert(max_vertex_size_ > 0);

    // TODO(dr): If we're relying on vertex pulling when using geometry streams, could
    // alternatively skip dynamic offsets and just provide storage buffer offsets as uniforms

    WGPUBindGroupEntry entries[num_vertex_slots]{};
    for (u32 i = 0; i < num_vertex_slots; ++i)
    {
        entries[i] = {
            .binding = i,
            .buffer = vertex_stage_.device_buf,
            .size = max_vertex_size_,
        };
    }
    WGPUBindGroupDescriptor const desc{
        .layout = bindings_layout,
        .entryCount = num_vertex_slots,
        .entries = entries,
    };
    vertex_bg_ = wgpuDeviceCreateBindGroup(device, &desc);
    bound_vertex_size_ = max_vertex_size_;
}

void GeometryStream::update_device_buffers(WGPUDevice const device, WGPUQueue const queue)
{
    if (!vertex_stage_.host_buf.empty())
    {
        // Pad the host buffer to account for the largest binding view when using dynamic offsets
        vertex_stage_.host_buf.resize(last_vertex_offset_ + max_vertex_size_);

        // Rebuild bindings if the vertex buffer was reallocated or its bind size has changed
        bool bindings_dirty = vertex_stage_.update_device(device, queue, WGPUBufferUsage_Storage);
        bindings_dirty |= (max_vertex_size_ != bound_vertex_size_);

        if (bindings_dirty)
            rebuild_bindings(device);
    }

    index_stage_.update_device(device, queue, WGPUBufferUsage_Index);
}

void GeometryStream::clear()
{
    vertex_stage_.host_buf.clear();
    index_stage_.host_buf.clear();
    vertex_offsets_.clear();
    index_offsets_.clear();
    max_vertex_size_ = 0;
    last_vertex_offset_ = 0;
}

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
