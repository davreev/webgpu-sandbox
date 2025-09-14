#include "utils.h"

WGPUBuffer make_buffer(WGPUDevice const device, size_t const size, WGPUBufferUsage const usage)
{
    return wgpuDeviceCreateBuffer(
        device,
        &(WGPUBufferDescriptor){
            .usage = usage,
            .size = size,
        });
}
