#include "gpu_resource.hpp"

namespace wgpu::sandbox
{

#define GPU_RESOURCE_RELEASE(Type_)                                                                \
    template <>                                                                                    \
    void Gpu##Type_::release()                                                                     \
    {                                                                                              \
        if (handle_)                                                                               \
        {                                                                                          \
            wgpu##Type_##Release(handle_);                                                         \
            handle_ = {};                                                                          \
        }                                                                                          \
    }

GPU_RESOURCE_RELEASE(Adapter)
GPU_RESOURCE_RELEASE(BindGroup)
GPU_RESOURCE_RELEASE(BindGroupLayout)
GPU_RESOURCE_RELEASE(Buffer)
GPU_RESOURCE_RELEASE(CommandBuffer)
GPU_RESOURCE_RELEASE(CommandEncoder)
GPU_RESOURCE_RELEASE(ComputePipeline)
GPU_RESOURCE_RELEASE(Device)
GPU_RESOURCE_RELEASE(Instance)
GPU_RESOURCE_RELEASE(PipelineLayout)
GPU_RESOURCE_RELEASE(QuerySet)
GPU_RESOURCE_RELEASE(Queue)
GPU_RESOURCE_RELEASE(RenderBundle)
GPU_RESOURCE_RELEASE(RenderBundleEncoder)
GPU_RESOURCE_RELEASE(RenderPipeline)
GPU_RESOURCE_RELEASE(Sampler)
GPU_RESOURCE_RELEASE(ShaderModule)
GPU_RESOURCE_RELEASE(Surface)
GPU_RESOURCE_RELEASE(Texture)
GPU_RESOURCE_RELEASE(TextureView)

#undef GPU_RESOURCE_RELEASE

#define GPU_RESOURCE_ADD_REF(Type_)                                                                \
    template <>                                                                                    \
    void Gpu##Type_::add_ref() const                                                               \
    {                                                                                              \
        if (handle_)                                                                               \
            wgpu##Type_##AddRef(handle_);                                                          \
    }

GPU_RESOURCE_ADD_REF(Adapter)
GPU_RESOURCE_ADD_REF(BindGroup)
GPU_RESOURCE_ADD_REF(BindGroupLayout)
GPU_RESOURCE_ADD_REF(Buffer)
GPU_RESOURCE_ADD_REF(CommandBuffer)
GPU_RESOURCE_ADD_REF(CommandEncoder)
GPU_RESOURCE_ADD_REF(ComputePipeline)
GPU_RESOURCE_ADD_REF(Device)
GPU_RESOURCE_ADD_REF(Instance)
GPU_RESOURCE_ADD_REF(PipelineLayout)
GPU_RESOURCE_ADD_REF(QuerySet)
GPU_RESOURCE_ADD_REF(Queue)
GPU_RESOURCE_ADD_REF(RenderBundle)
GPU_RESOURCE_ADD_REF(RenderBundleEncoder)
GPU_RESOURCE_ADD_REF(RenderPipeline)
GPU_RESOURCE_ADD_REF(Sampler)
GPU_RESOURCE_ADD_REF(ShaderModule)
GPU_RESOURCE_ADD_REF(Surface)
GPU_RESOURCE_ADD_REF(Texture)
GPU_RESOURCE_ADD_REF(TextureView)

#undef GPU_RESOURCE_ADD_REF

#define GPU_PASS_ENCODER_RELEASE(Type_)                                                            \
    template <>                                                                                    \
    void Gpu##Type_::release()                                                                     \
    {                                                                                              \
        if (handle_)                                                                               \
        {                                                                                          \
            wgpu##Type_##End(handle_);                                                             \
            wgpu##Type_##Release(handle_);                                                         \
            handle_ = {};                                                                          \
        }                                                                                          \
    }

GPU_PASS_ENCODER_RELEASE(RenderPassEncoder)
GPU_PASS_ENCODER_RELEASE(ComputePassEncoder)

#undef GPU_PASS_ENCODER_RELEASE

} // namespace wgpu::sandbox
