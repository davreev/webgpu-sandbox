#include "wgpu_config.h"
#include "webgpu/webgpu.h"

WGPUBuffer make_buffer(WGPUDevice const device, size_t const size, WGPUBufferUsageFlags const usage)
{
    WGPUBufferDescriptor const desc = {
        .usage = usage,
        .size = size,
    };

    return wgpuDeviceCreateBuffer(device, &desc);
}
