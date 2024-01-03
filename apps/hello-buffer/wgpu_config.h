#ifndef WGPU_CONFIG_INCLUDED
#define WGPU_CONFIG_INCLUDED

#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C"
{
#endif

WGPUBuffer make_buffer(WGPUDevice device, size_t size, WGPUBufferUsageFlags usage);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WGPU_CONFIG_INCLUDED
