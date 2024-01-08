#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include <wgpu_utils.hpp>

#include "shader_src.hpp"
#include "wgpu_config.h"

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu;

    // Initialize GLFW
    if (!glfwInit())
    {
        fmt::print("Failed to initialize GLFW\n");
        return 1;
    }
    auto const deinit_glfw = defer([]() { glfwTerminate(); });

    // Create GLFW window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* const window = glfwCreateWindow(640, 480, "Hello Triangle", nullptr, nullptr);
    if (!window)
    {
        fmt::print("Failed to create window\n");
        return 1;
    }
    auto const drop_window = defer([=]() { glfwDestroyWindow(window); });

    // Create WebGPU instance
    WGPUInstance const instance = wgpuCreateInstance(nullptr);
    if (!instance)
    {
        fmt::print("Failed to create WebGPU instance\n");
        return 1;
    }
    auto const drop_instance = defer([=]() { wgpuInstanceRelease(instance); });

    // Get WebGPU surface from GLFW window
    WGPUSurface const surface = make_surface(instance, window);
    if (!surface)
    {
        fmt::print("Failed to get WebGPU surface\n");
        return 1;
    }
    auto const drop_surface = defer([=]() { wgpuSurfaceRelease(surface); });

    // Create WebGPU adapter
    WGPUAdapter adapter{};
    {
        WGPURequestAdapterOptions options{};
        {
            options.compatibleSurface = surface;
            options.powerPreference = WGPUPowerPreference_HighPerformance;
        }
        adapter = request_adapter(instance, &options);
    }
    if (!adapter)
    {
        fmt::print("Failed to get WebGPU adapter\n");
        return 1;
    }
    auto const drop_adapter = defer([=]() { wgpuAdapterRelease(adapter); });

    // Create WebGPU device
    WGPUDevice const device = request_device(adapter);
    if (!device)
    {
        fmt::print("Failed to get WebGPU device\n");
        return 1;
    }
    auto const drop_device = defer([=]() { wgpuDeviceRelease(device); });

    // Set error callback on device
    constexpr auto device_error_cb =
        [](WGPUErrorType type, char const* message, void* /*userdata*/) {
            fmt::print(
                "Device error: {} ({})\nMessage: {}\n",
                to_string(type),
                static_cast<int>(type),
                message);
        };
    wgpuDeviceSetUncapturedErrorCallback(device, device_error_cb, nullptr);

    // Configure surface (replaces swap chain API)
    // WGPUTextureFormat const surface_fmt = wgpuSurfaceGetPreferredFormat(surface, adapter);
    constexpr WGPUTextureFormat surface_fmt = WGPUTextureFormat_BGRA8Unorm;
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        WGPUSurfaceConfiguration config{};
        config.device = device;
        config.width = width;
        config.height = height;
        config.format = surface_fmt;
        config.usage = WGPUTextureUsage_RenderAttachment;
        config.presentMode = WGPUPresentMode_Mailbox;

        wgpuSurfaceConfigure(surface, &config);
    }
    auto const unconfig_srf = defer([=] { wgpuSurfaceUnconfigure(surface); });

    // Create render pipeline
    WGPURenderPipeline pipeline{};
    {
        // Create shader module
        WGPUShaderModule const shader = make_shader_module(device, hello_triangle::shader_src);
        auto const drop_shader = defer([=]() { wgpuShaderModuleRelease(shader); });

        pipeline = make_render_pipeline(device, shader, surface_fmt);
    }
    auto const drop_pipeline = defer([=]() { wgpuRenderPipelineRelease(pipeline); });

    // Get the device's queue
    WGPUQueue const queue = wgpuDeviceGetQueue(device);

    // Frame loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Create a command encoder from the device
        WGPUCommandEncoder const encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
        auto const drop_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // Render pass
        {
            // Get the render target for the current frame
            WGPUSurfaceTexture const curr_tex = get_current_texture(surface);
            if (curr_tex.status != WGPUSurfaceGetCurrentTextureStatus_Success)
            {
                fmt::print("Failed to get surface texture ({})\n", to_string(curr_tex.status));
                return 1;
            }

            WGPUTextureView const tex_view = make_texture_view(curr_tex);
            auto const drop_tex_view = defer([=]() { wgpuTextureViewRelease(tex_view); });

            WGPURenderPassEncoder const pass = begin_render_pass(encoder, tex_view);
            auto const end_pass = defer([=]() { wgpuRenderPassEncoderEnd(pass); });

            wgpuRenderPassEncoderSetPipeline(pass, pipeline);
            wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);
        }

        // Create a command via the encoder
        WGPUCommandBuffer const command = wgpuCommandEncoderFinish(encoder, nullptr);
        auto const drop_command = defer([=]() { wgpuCommandBufferRelease(command); });

        // Submit encoded command
        wgpuQueueSubmit(queue, 1, &command);

        // Display the result
        wgpuSurfacePresent(surface);
    }

    return 0;
}