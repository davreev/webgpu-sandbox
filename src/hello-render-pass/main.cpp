#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include <wgpu_utils.hpp>

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
    GLFWwindow* const window = glfwCreateWindow(640, 480, "Hello Render Pass", nullptr, nullptr);
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

    // Create WebGPU adapter via the instance
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

    // Report adapter details
    report_features(adapter);
    report_limits(adapter);
    report_properties(adapter);
    report_surface_capabilities(surface, adapter);

    // Create WebGPU device via the adapter
    WGPUDevice const device = request_device(adapter);
    if (!device)
    {
        fmt::print("Failed to get WebGPU device\n");
        return 1;
    }
    auto const drop_device = defer([=]() { wgpuDeviceRelease(device); });

    // Report device details
    report_features(device);
    report_limits(device);

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
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        WGPUSurfaceConfiguration config{};
        config.device = device;
        config.width = width;
        config.height = height;
        // config.format = wgpuSurfaceGetPreferredFormat(surface, adapter);
        config.format = WGPUTextureFormat_BGRA8Unorm;
        config.usage = WGPUTextureUsage_RenderAttachment;
        config.presentMode = WGPUPresentMode_Mailbox;

        wgpuSurfaceConfigure(surface, &config);
    }
    auto const unconfig_srf = defer([=] { wgpuSurfaceUnconfigure(surface); });

    // Get the device's queue
    WGPUQueue const queue = wgpuDeviceGetQueue(device);

    struct FrameInfo
    {
        std::size_t count;
        // ...
    } frame_info{};

    // Frame loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Create a command encoder from the device
        WGPUCommandEncoder const encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
        auto const drop_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // Can use debug markers on the encoder for printf-like debugging bw commands
        // wgpuCommandEncoderInsertDebugMarker(encoder, "Do a thing");
        // wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

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
            auto const drop_view = defer([=]() { wgpuTextureViewRelease(tex_view); });

            WGPURenderPassEncoder const pass = begin_render_pass(encoder, tex_view);
            auto const end_pass = defer([=]() { wgpuRenderPassEncoderEnd(pass); });

            // NOTE(dr): Render pass clears the screen by default
        }

        // Create a command via the encoder
        WGPUCommandBuffer const command = wgpuCommandEncoderFinish(encoder, nullptr);
        auto const drop_command = defer([=]() { wgpuCommandBufferRelease(command); });

        // Submit encoded command
        wgpuQueueSubmit(queue, 1, &command);

        // Register callback that fires when queued work is done
        constexpr auto work_done_cb = [](WGPUQueueWorkDoneStatus const status, void* userdata) {
            auto const frame_info = static_cast<FrameInfo*>(userdata);
            if (frame_info->count % 100 == 0)
            {
                fmt::print(
                    "Finished frame {} with status: {}\n",
                    frame_info->count,
                    to_string(status));
            }
            ++frame_info->count;
        };
        wgpuQueueOnSubmittedWorkDone(queue, work_done_cb, &frame_info);

        // Display the result
        wgpuSurfacePresent(surface);
    }

    return 0;
}
