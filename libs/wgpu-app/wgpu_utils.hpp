#pragma once

#include <cassert>

#include <GLFW/glfw3.h>

#include <webgpu/webgpu.h>

namespace wgpu
{

void raise_event(char const* name);

void wait_for_event(char const* name);

#ifdef __EMSCRIPTEN__
void get_canvas_client_size(int& width, int& height);
#endif

#ifdef __EMSCRIPTEN__
WGPUSurface make_surface(WGPUInstance const instance, char const* canvas_selector);
#else
WGPUSurface make_surface(WGPUInstance instance, GLFWwindow* window);
#endif

WGPUAdapter request_adapter(
    WGPUInstance instance,
    WGPURequestAdapterOptions const* options = nullptr);

WGPUDevice request_device(WGPUAdapter adapter, WGPUDeviceDescriptor const* desc = nullptr);

WGPUTextureFormat get_preferred_texture_format(WGPUSurface surface, WGPUAdapter adapter);

void report_adapter_features(WGPUAdapter adapter);

void report_adapter_limits(WGPUAdapter adapter);

void report_adapter_properties(WGPUAdapter adapter);

void report_device_features(WGPUDevice device);

void report_device_limits(WGPUDevice device);

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

char const* to_string(WGPUMapAsyncStatus value);

} // namespace wgpu
