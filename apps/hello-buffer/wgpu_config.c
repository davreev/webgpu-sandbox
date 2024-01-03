#include "wgpu_config.h"
#include "webgpu/webgpu.h"

WGPUBuffer make_buffer(WGPUDevice device, size_t size, WGPUBufferUsageFlags usage)
{
    WGPUBufferDescriptor const desc = {
        .usage = usage,
        .size = size,
    };

    return wgpuDeviceCreateBuffer(device, &desc);
}
