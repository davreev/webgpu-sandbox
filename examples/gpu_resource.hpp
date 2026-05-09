#pragma once

#include <webgpu/webgpu.h>

namespace wgpu::sandbox
{

template <typename Handle_>
struct GpuResource
{
    using Handle = Handle_;

    GpuResource() = default;
    GpuResource(Handle handle) : handle_{handle} {}
    GpuResource(GpuResource const& other) : handle_{other.handle_} { add_ref(); }
    GpuResource(GpuResource&& other) noexcept : handle_{other.handle_} { other.handle_ = {}; }

    GpuResource& operator=(GpuResource const& other)
    {
        if (this != &other)
        {
            release();
            handle_ = other.handle_;
            add_ref();
        }
        return *this;
    }

    GpuResource& operator=(GpuResource&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_ = other.handle_;
            other.handle_ = {};
        }
        return *this;
    }

    ~GpuResource() { release(); }

    operator Handle() const { return handle_; }
    Handle handle() const { return handle_; }

  private:
    Handle handle_{};

    void release();
    void add_ref() const;
};

using GpuAdapter = GpuResource<WGPUAdapter>;
using GpuBindGroup = GpuResource<WGPUBindGroup>;
using GpuBindGroupLayout = GpuResource<WGPUBindGroupLayout>;
using GpuBuffer = GpuResource<WGPUBuffer>;
using GpuCommandBuffer = GpuResource<WGPUCommandBuffer>;
using GpuCommandEncoder = GpuResource<WGPUCommandEncoder>;
using GpuComputePipeline = GpuResource<WGPUComputePipeline>;
using GpuDevice = GpuResource<WGPUDevice>;
using GpuInstance = GpuResource<WGPUInstance>;
using GpuPipelineLayout = GpuResource<WGPUPipelineLayout>;
using GpuQuerySet = GpuResource<WGPUQuerySet>;
using GpuQueue = GpuResource<WGPUQueue>;
using GpuRenderBundle = GpuResource<WGPURenderBundle>;
using GpuRenderBundleEncoder = GpuResource<WGPURenderBundleEncoder>;
using GpuRenderPipeline = GpuResource<WGPURenderPipeline>;
using GpuSampler = GpuResource<WGPUSampler>;
using GpuShaderModule = GpuResource<WGPUShaderModule>;
using GpuSurface = GpuResource<WGPUSurface>;
using GpuTexture = GpuResource<WGPUTexture>;
using GpuTextureView = GpuResource<WGPUTextureView>;

// NOTE(dr): Pass encoders differ from other resource types in that they must be ended before
// release, which precludes shared ownership via ref counting. Copy is therefore not supported.

template <typename Handle_>
struct GpuPassEncoder
{
    using Handle = Handle_;

    GpuPassEncoder() = default;
    GpuPassEncoder(Handle handle) : handle_{handle} {}
    GpuPassEncoder(GpuPassEncoder&& other) noexcept : handle_{other.handle_} { other.handle_ = {}; }

    GpuPassEncoder& operator=(GpuPassEncoder&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_ = other.handle_;
            other.handle_ = {};
        }
        return *this;
    }

    ~GpuPassEncoder() { release(); }

    operator Handle() const { return handle_; }
    Handle handle() const { return handle_; }

  private:
    Handle handle_{};

    void release();
};

using GpuRenderPassEncoder = GpuPassEncoder<WGPURenderPassEncoder>;
using GpuComputePassEncoder = GpuPassEncoder<WGPUComputePassEncoder>;

} // namespace wgpu::sandbox
