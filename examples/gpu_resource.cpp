#include "gpu_resource.hpp"

namespace wgpu::sandbox
{

template <>
void GpuAdapter::release()
{
    if (handle_)
    {
        wgpuAdapterRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuAdapter::add_ref() const
{
    if (handle_)
        wgpuAdapterAddRef(handle_);
}

template <>
void GpuBindGroup::release()
{
    if (handle_)
    {
        wgpuBindGroupRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuBindGroup::add_ref() const
{
    if (handle_)
        wgpuBindGroupAddRef(handle_);
}

template <>
void GpuBindGroupLayout::release()
{
    if (handle_)
    {
        wgpuBindGroupLayoutRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuBindGroupLayout::add_ref() const
{
    if (handle_)
        wgpuBindGroupLayoutAddRef(handle_);
}

template <>
void GpuBuffer::release()
{
    if (handle_)
    {
        wgpuBufferRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuBuffer::add_ref() const
{
    if (handle_)
        wgpuBufferAddRef(handle_);
}

template <>
void GpuCommandBuffer::release()
{
    if (handle_)
    {
        wgpuCommandBufferRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuCommandBuffer::add_ref() const
{
    if (handle_)
        wgpuCommandBufferAddRef(handle_);
}

template <>
void GpuCommandEncoder::release()
{
    if (handle_)
    {
        wgpuCommandEncoderRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuCommandEncoder::add_ref() const
{
    if (handle_)
        wgpuCommandEncoderAddRef(handle_);
}

template <>
void GpuComputePipeline::release()
{
    if (handle_)
    {
        wgpuComputePipelineRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuComputePipeline::add_ref() const
{
    if (handle_)
        wgpuComputePipelineAddRef(handle_);
}

template <>
void GpuDevice::release()
{
    if (handle_)
    {
        wgpuDeviceRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuDevice::add_ref() const
{
    if (handle_)
        wgpuDeviceAddRef(handle_);
}

template <>
void GpuInstance::release()
{
    if (handle_)
    {
        wgpuInstanceRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuInstance::add_ref() const
{
    if (handle_)
        wgpuInstanceAddRef(handle_);
}

template <>
void GpuPipelineLayout::release()
{
    if (handle_)
    {
        wgpuPipelineLayoutRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuPipelineLayout::add_ref() const
{
    if (handle_)
        wgpuPipelineLayoutAddRef(handle_);
}

template <>
void GpuQuerySet::release()
{
    if (handle_)
    {
        wgpuQuerySetRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuQuerySet::add_ref() const
{
    if (handle_)
        wgpuQuerySetAddRef(handle_);
}

template <>
void GpuQueue::release()
{
    if (handle_)
    {
        wgpuQueueRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuQueue::add_ref() const
{
    if (handle_)
        wgpuQueueAddRef(handle_);
}

template <>
void GpuRenderBundle::release()
{
    if (handle_)
    {
        wgpuRenderBundleRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuRenderBundle::add_ref() const
{
    if (handle_)
        wgpuRenderBundleAddRef(handle_);
}

template <>
void GpuRenderBundleEncoder::release()
{
    if (handle_)
    {
        wgpuRenderBundleEncoderRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuRenderBundleEncoder::add_ref() const
{
    if (handle_)
        wgpuRenderBundleEncoderAddRef(handle_);
}

template <>
void GpuRenderPipeline::release()
{
    if (handle_)
    {
        wgpuRenderPipelineRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuRenderPipeline::add_ref() const
{
    if (handle_)
        wgpuRenderPipelineAddRef(handle_);
}

template <>
void GpuSampler::release()
{
    if (handle_)
    {
        wgpuSamplerRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuSampler::add_ref() const
{
    if (handle_)
        wgpuSamplerAddRef(handle_);
}

template <>
void GpuShaderModule::release()
{
    if (handle_)
    {
        wgpuShaderModuleRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuShaderModule::add_ref() const
{
    if (handle_)
        wgpuShaderModuleAddRef(handle_);
}

template <>
void GpuSurface::release()
{
    if (handle_)
    {
        wgpuSurfaceRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuSurface::add_ref() const
{
    if (handle_)
        wgpuSurfaceAddRef(handle_);
}

template <>
void GpuTexture::release()
{
    if (handle_)
    {
        wgpuTextureRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuTexture::add_ref() const
{
    if (handle_)
        wgpuTextureAddRef(handle_);
}

template <>
void GpuTextureView::release()
{
    if (handle_)
    {
        wgpuTextureViewRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuTextureView::add_ref() const
{
    if (handle_)
        wgpuTextureViewAddRef(handle_);
}

template <>
void GpuRenderPassEncoder::release()
{
    if (handle_)
    {
        wgpuRenderPassEncoderEnd(handle_);
        wgpuRenderPassEncoderRelease(handle_);
        handle_ = {};
    }
}

template <>
void GpuComputePassEncoder::release()
{
    if (handle_)
    {
        wgpuComputePassEncoderEnd(handle_);
        wgpuComputePassEncoderRelease(handle_);
        handle_ = {};
    }
}

} // namespace wgpu::sandbox
