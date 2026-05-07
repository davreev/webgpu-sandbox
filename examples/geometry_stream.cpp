#include "geometry_stream.hpp"

#include <cassert>

#include <wgpu_default_limits.hpp>

namespace wgpu::sandbox
{
namespace
{

constexpr usize aligned_size(usize const size, usize const align)
{
    return (size + align - 1) & ~(align - 1);
}

constexpr u32 num_vertex_buffers = default_max_dynamic_storage_buffers_per_pipeline_layout;
constexpr u32 vertex_buffer_alignment = default_min_storage_buffer_offset_alignment;

WGPUBindGroupLayout bg_layout{};

} // namespace

void GeometryStream::init_shared_resources(WGPUDevice const device)
{
    if (bg_layout)
        wgpuBindGroupLayoutRelease(bg_layout);

    WGPUBindGroupLayoutEntry entries[num_vertex_buffers]{};
    for (u32 i = 0; i < num_vertex_buffers; ++i)
    {
        entries[i] = {
            .binding = i,
            .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .buffer{
                .type = WGPUBufferBindingType_ReadOnlyStorage,
                .hasDynamicOffset = true,
                .minBindingSize = vertex_buffer_alignment,
            },
        };
    }
    WGPUBindGroupLayoutDescriptor const desc{
        .entryCount = num_vertex_buffers,
        .entries = entries,
    };
    bg_layout = wgpuDeviceCreateBindGroupLayout(device, &desc);
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
    return vertex_stage_.append(data, vertex_buffer_alignment);
}

u32 GeometryStream::push_vertices_once(void const* key, u8 const slot, Span<u8 const> const& data)
{
    assert(slot < num_vertex_buffers);

    auto const [it, ok] = vertex_offsets_.try_emplace({key, slot});
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
    if (bg_)
        wgpuBindGroupRelease(bg_);

    WGPUBindGroupEntry entries[num_vertex_buffers]{};
    for (u32 i = 0; i < num_vertex_buffers; ++i)
    {
        entries[i] = {
            .binding = i,
            .buffer = vertex_stage_.device_buf,
            .size = WGPU_WHOLE_SIZE,
        };
    }
    WGPUBindGroupDescriptor const desc{
        .layout = bg_layout,
        .entryCount = num_vertex_buffers,
        .entries = entries,
    };
    bg_ = wgpuDeviceCreateBindGroup(device, &desc);
}

void GeometryStream::update_device_buffers(WGPUDevice const device, WGPUQueue const queue)
{
    if (vertex_stage_.update_device(device, queue, WGPUBufferUsage_Storage))
        rebuild_bindings(device);

    index_stage_.update_device(device, queue, WGPUBufferUsage_Index);
}

void GeometryStream::clear()
{
    vertex_stage_.host_buf.clear();
    index_stage_.host_buf.clear();
    vertex_offsets_.clear();
    index_offsets_.clear();
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

        if (device_buf)
            wgpuBufferRelease(device_buf);

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
