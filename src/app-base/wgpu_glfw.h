#ifndef WEBGPU_GLFW_H
#define WEBGPU_GLFW_H

#include <GLFW/glfw3.h>

#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C"
{
#endif

WGPUSurface wgpu_make_surface_from_glfw(WGPUInstance instance, GLFWwindow* window);

#ifdef __cplusplus
}
#endif

#endif // WEBGPU_GLFW_H
