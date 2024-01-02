#pragma once

#include <cassert>
#include <utility>
#include <vector>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

namespace wgpu
{

template <typename T>
struct Range
{
    T* ptr;
    std::size_t size;
    T* begin() const { return ptr; }
    T* end() const { return ptr + size; }
};

template <typename Func>
struct Deferred
{
    template <typename Func_>
    Deferred(Func_&& func) : func_(std::forward<Func_>(func))
    {
    }

    Deferred(Deferred const&) = delete;
    Deferred& operator=(Deferred const&) = delete;

    ~Deferred() { func_(); }

  private:
    Func func_;
};

template <typename Func>
[[nodiscard]] Deferred<Func> defer(Func&& func)
{
    return {std::forward<Func>(func)};
}

WGPUAdapter request_adapter(WGPUInstance instance, WGPURequestAdapterOptions const* options);

WGPUDevice request_device(WGPUAdapter adapter, WGPUDeviceDescriptor const* desc);

void report_features(WGPUAdapter adapter);

void report_features(WGPUDevice device);

void report_limits(WGPUAdapter adapter);

void report_limits(WGPUDevice device);

void report_properties(WGPUAdapter adapter);

void report_surface_capabilities(WGPUSurface surface, WGPUAdapter adapter);

char const* to_string(WGPUFeatureName value);

char const* to_string(WGPUAdapterType value);

char const* to_string(WGPUBackendType value);

char const* to_string(WGPUErrorType value);

char const* to_string(WGPUQueueWorkDoneStatus value);

char const* to_string(WGPUSurfaceGetCurrentTextureStatus value);

char const* to_string(WGPUTextureFormat value);

char const* to_string(WGPUCompositeAlphaMode value);

char const* to_string(WGPUPresentMode value);

} // namespace wgpu