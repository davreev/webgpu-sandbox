#ifndef WEBGPU_UTIL_GLFW_H
#define WEBGPU_UTIL_GLFW_H

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C"
{
#endif

/// Get a WGPUSurface from a GLFW window
WGPUSurface glfwGetWGPUSurface(WGPUInstance instance, GLFWwindow* window);

#ifdef __cplusplus
}
#endif

#endif // WEBGPU_UTIL_GLFW_H