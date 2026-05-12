#include "gpu_resource.hpp"
#include "webgpu/webgpu.h"

namespace wgpu::sandbox
{
namespace
{

template <typename Handle>
void wgpu_resource_release(Handle handle);

template <typename Handle>
void wgpu_resource_add_ref(Handle handle);

template <typename Handle>
void wgpu_pass_encoder_end(Handle handle);

#define WGPU_RESOURCE_IMPL(name_)                                                                  \
    template <>                                                                                    \
    void wgpu_resource_release<WGPU##name_>(WGPU##name_ h)                                         \
    {                                                                                              \
        wgpu##name_##Release(h);                                                                   \
    }                                                                                              \
    template <>                                                                                    \
    void wgpu_resource_add_ref<WGPU##name_>(WGPU##name_ h)                                         \
    {                                                                                              \
        wgpu##name_##AddRef(h);                                                                    \
    }

WGPU_RESOURCE_IMPL(Adapter)
WGPU_RESOURCE_IMPL(BindGroup)
WGPU_RESOURCE_IMPL(BindGroupLayout)
WGPU_RESOURCE_IMPL(Buffer)
WGPU_RESOURCE_IMPL(CommandBuffer)
WGPU_RESOURCE_IMPL(CommandEncoder)
WGPU_RESOURCE_IMPL(ComputePipeline)
WGPU_RESOURCE_IMPL(Device)
WGPU_RESOURCE_IMPL(Instance)
WGPU_RESOURCE_IMPL(PipelineLayout)
WGPU_RESOURCE_IMPL(QuerySet)
WGPU_RESOURCE_IMPL(Queue)
WGPU_RESOURCE_IMPL(RenderBundle)
WGPU_RESOURCE_IMPL(RenderBundleEncoder)
WGPU_RESOURCE_IMPL(RenderPipeline)
WGPU_RESOURCE_IMPL(Sampler)
WGPU_RESOURCE_IMPL(ShaderModule)
WGPU_RESOURCE_IMPL(Surface)
WGPU_RESOURCE_IMPL(Texture)
WGPU_RESOURCE_IMPL(TextureView)

#undef WGPU_RESOURCE_IMPL

#define WGPU_PASS_ENCODER_IMPL(name_)                                                              \
    template <>                                                                                    \
    void wgpu_resource_release<WGPU##name_>(WGPU##name_ h)                                         \
    {                                                                                              \
        wgpu##name_##Release(h);                                                                   \
    }                                                                                              \
    template <>                                                                                    \
    void wgpu_pass_encoder_end(WGPU##name_ h)                                                      \
    {                                                                                              \
        wgpu##name_##End(h);                                                                       \
    }

WGPU_PASS_ENCODER_IMPL(RenderPassEncoder)
WGPU_PASS_ENCODER_IMPL(ComputePassEncoder)

#undef WGPU_PASS_ENCODER_IMPL

} // namespace

template <typename Handle>
void GpuResource<Handle>::release()
{
    if (handle_)
    {
        wgpu_resource_release(handle_);
        handle_ = {};
    }
}

template <typename Handle>
void GpuResource<Handle>::add_ref() const
{
    if (handle_)
        wgpu_resource_add_ref(handle_);
}

template struct GpuResource<WGPUAdapter>;
template struct GpuResource<WGPUBindGroup>;
template struct GpuResource<WGPUBindGroupLayout>;
template struct GpuResource<WGPUBuffer>;
template struct GpuResource<WGPUCommandBuffer>;
template struct GpuResource<WGPUCommandEncoder>;
template struct GpuResource<WGPUComputePipeline>;
template struct GpuResource<WGPUDevice>;
template struct GpuResource<WGPUInstance>;
template struct GpuResource<WGPUPipelineLayout>;
template struct GpuResource<WGPUQuerySet>;
template struct GpuResource<WGPUQueue>;
template struct GpuResource<WGPURenderBundle>;
template struct GpuResource<WGPURenderBundleEncoder>;
template struct GpuResource<WGPURenderPipeline>;
template struct GpuResource<WGPUSampler>;
template struct GpuResource<WGPUShaderModule>;
template struct GpuResource<WGPUSurface>;
template struct GpuResource<WGPUTexture>;
template struct GpuResource<WGPUTextureView>;

template <typename Handle>
void GpuPassEncoder<Handle>::release()
{
    if (handle_)
    {
        wgpu_pass_encoder_end(handle_);
        wgpu_resource_release(handle_);
        handle_ = {};
    }
}

template struct GpuPassEncoder<WGPURenderPassEncoder>;
template struct GpuPassEncoder<WGPUComputePassEncoder>;

} // namespace wgpu::sandbox
