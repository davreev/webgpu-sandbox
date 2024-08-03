#include "wgpu_config.h"
#include "webgpu/webgpu.h"

WGPUBuffer make_buffer(WGPUDevice const device, size_t const size, WGPUBufferUsageFlags const usage)
{
    return wgpuDeviceCreateBuffer(
        device,
        &(WGPUBufferDescriptor){
            .usage = usage,
            .size = size,
        });
}
