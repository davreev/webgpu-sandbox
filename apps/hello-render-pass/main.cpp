#include <cassert>
#include <vector>

#include <fmt/core.h>

#include <webgpu/app.hpp>
#include <webgpu/glfw.h>
#include <webgpu/webgpu.h>

namespace wgpu
{

void device_error_cb(WGPUErrorType type, char const* message, void* /*userdata*/)
{
    fmt::print(
        "Device error: {} ({})\nMessage: {}\n",
        to_string(type),
        static_cast<int>(type),
        message);
}

} // namespace wgpu

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu;

    constexpr int window_width{640};
    constexpr int window_height{480};

    // Create WebGPU instance
    WGPUInstance instance{};
    {
        WGPUInstanceDescriptor desc{};
        instance = wgpuCreateInstance(&desc);
        if (!instance)
        {
            fmt::print("Failed to create WebGPU instance\n");
            return 1;
        }
    }
    auto const rel_instance = defer([=]() { wgpuInstanceRelease(instance); });

    // Initialize GLFW
    if (!glfwInit())
    {
        fmt::print("Failed to initialize GLFW\n");
        return 1;
    }
    auto const rel_glfw = defer([]() { glfwTerminate(); });

    // Create GLFW window
    GLFWwindow* window{};
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow( //
            window_width,
            window_height,
            "Learn WebGPU C++",
            nullptr,
            nullptr);

        if (!window)
        {
            fmt::print("Failed to create window\n");
            return 1;
        }
    }
    auto const rel_window = defer([=]() { glfwDestroyWindow(window); });

    // Get WebGPU surface from GLFW window
    WGPUSurface const surface = glfwGetWGPUSurface(instance, window);
    if (!surface)
    {
        fmt::print("Failed to get WebGPU surface\n");
        return 1;
    }
    auto const rel_surface = defer([=]() { wgpuSurfaceRelease(surface); });

    // Create WebGPU adapter
    WGPUAdapter adapter{};
    {
        WGPURequestAdapterOptions options{};
        options.compatibleSurface = surface;
        adapter = request_adapter(instance, &options);
        if (!adapter)
        {
            fmt::print("Failed to get WebGPU adapter\n");
            return 1;
        }

        report_features(adapter);
        report_limits(adapter);
        report_properties(adapter);
        report_surface_capabilities(surface, adapter);
    }
    auto const rel_adapter = defer([=]() { wgpuAdapterRelease(adapter); });

    // Create WebGPU device
    WGPUDevice device{};
    {
        WGPUDeviceDescriptor desc{};
        desc.label = "My device";
        // desc.requiredFeatureCount = 0;
        // desc.requiredLimits = nullptr;
        // desc.defaultQueue.nextInChain = nullptr;
        desc.defaultQueue.label = "The default queue";

        device = request_device(adapter, &desc);
        if (!device)
        {
            fmt::print("Failed to get WebGPU device\n");
            return 1;
        }

        wgpuDeviceSetUncapturedErrorCallback(device, device_error_cb, nullptr);

        report_features(device);
        report_limits(device);
    }
    auto const rel_device = defer([=]() { wgpuDeviceRelease(device); });

    // Get the device's queue
    WGPUQueue const queue = wgpuDeviceGetQueue(device);
    {
        // Add callback which fires whenever queued work is done
        auto const work_done_cb = [](WGPUQueueWorkDoneStatus const status, void* /*userdata*/)
        { fmt::print("Queued work finished with status: {}\n", to_string(status)); };

        wgpuQueueOnSubmittedWorkDone(queue, work_done_cb, nullptr);
    }

    // Configure surface via the device (replaces swap chain API)
    WGPUSurfaceConfiguration config{};
    {
        config.device = device;
        config.width = window_width;
        config.height = window_height;
        config.format = wgpuSurfaceGetPreferredFormat(surface, adapter);
        config.usage = WGPUTextureUsage_RenderAttachment;
        config.presentMode = WGPUPresentMode_Mailbox;
    }
    wgpuSurfaceConfigure(surface, &config);
    auto const unconfig_srf = defer([&] { wgpuSurfaceUnconfigure(surface); });

    // Start frame loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Get the texture to render the frame to
        WGPUSurfaceTexture srf_tex;
        wgpuSurfaceGetCurrentTexture(surface, &srf_tex);
        if (srf_tex.status != WGPUSurfaceGetCurrentTextureStatus_Success)
        {
            fmt::print("Failed to get surface texture ({})\n", to_string(srf_tex.status));
            return 1;
        }

        // Create a command encoder from the device
        WGPUCommandEncoder encoder{};
        {
            WGPUCommandEncoderDescriptor desc{};
            desc.label = "My encoder";
            encoder = wgpuDeviceCreateCommandEncoder(device, &desc);
        }
        auto const rel_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // Can use debug markers on the encoder for printf-like debugging bw commands
        // wgpuCommandEncoderInsertDebugMarker(encoder, "Do a thing");
        // wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

        // Create render pass via the encoder
        WGPURenderPassEncoder pass{};
        {
            WGPUTextureView view{};
            {
                WGPUTextureViewDescriptor desc{};
                {
                    desc.mipLevelCount = 1;
                    desc.arrayLayerCount = 1;
                }
                view = wgpuTextureCreateView(srf_tex.texture, &desc);
                assert(view);
            }
            // NOTE(dr): TextureView doesn't need to be released!
            // auto const rel_view = defer([&]() { wgpuTextureViewRelease(view); });

            WGPURenderPassColorAttachment color{};
            {
                color.view = view;
                color.loadOp = WGPULoadOp_Clear;
                color.storeOp = WGPUStoreOp_Store;
                color.clearValue = WGPUColor{1.0, 0.3, 0.3, 1.0};
            }

            WGPURenderPassDescriptor desc{};
            {
                desc.colorAttachments = &color;
                desc.colorAttachmentCount = 1;
                // ...
            }

            pass = wgpuCommandEncoderBeginRenderPass(encoder, &desc);
        }

        // NOTE(dr): Render pass clears the screen by default
        // ...
        // ...
        // ...

        wgpuRenderPassEncoderEnd(pass);

        // Create a command via the encoder
        WGPUCommandBuffer command{};
        {
            WGPUCommandBufferDescriptor desc{};
            desc.label = "My command buffer";
            command = wgpuCommandEncoderFinish(encoder, &desc);
        }

        // Submit encoded command
        wgpuQueueSubmit(queue, 1, &command);

        // Display the result
        wgpuSurfacePresent(surface);
    }

    return 0;
}
