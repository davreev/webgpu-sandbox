#ifndef UTILS_H
#define UTILS_H

#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C"
{
#endif

WGPUBuffer make_buffer(WGPUDevice device, size_t size, WGPUBufferUsageFlags usage);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UTILS_H
