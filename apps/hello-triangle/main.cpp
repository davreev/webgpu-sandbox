#include <cassert>

#include <fmt/core.h>

#include <webgpu/app.hpp>
#include <webgpu/glfw.h>
#include <webgpu/webgpu.h>

#include "gfx_config.h"

namespace wgpu
{

constexpr char const* shader_src = R"(
@vertex
fn vs_main(@builtin(vertex_index) v: u32) -> @builtin(position) vec4f {
    // TODO(dr): Use lookup table
    if(v == 0u){
        return vec4f(-0.5, -0.5, 0.0, 1.0);
    } else if(v == 1u){
        return vec4f(0.5, -0.5, 0.0, 1.0);
    } else if(v == 2u){
        return vec4f(0.0, 0.5, 0.0, 1.0);
    } else {
        return vec4f(0.0, 0.0, 0.0, 1.0);
    }
}

@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.0, 0.4, 1.0, 1.0);
}

)";

}

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu;

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
    auto const drop_instance = defer([=]() { wgpuInstanceRelease(instance); });

    // Initialize GLFW
    if (!glfwInit())
    {
        fmt::print("Failed to initialize GLFW\n");
        return 1;
    }
    auto const drop_glfw = defer([]() { glfwTerminate(); });

    // Create GLFW window
    GLFWwindow* window{};
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(640, 480, "Hello Triangle", nullptr, nullptr);
        if (!window)
        {
            fmt::print("Failed to create window\n");
            return 1;
        }
    }
    auto const drop_window = defer([=]() { glfwDestroyWindow(window); });

    // Get WebGPU surface from GLFW window
    WGPUSurface const surface = glfwGetWGPUSurface(instance, window);
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
        options.compatibleSurface = surface;
        adapter = request_adapter(instance, &options);
        if (!adapter)
        {
            fmt::print("Failed to get WebGPU adapter\n");
            return 1;
        }
    }
    auto const drop_adapter = defer([=]() { wgpuAdapterRelease(adapter); });

    // Create WebGPU device
    WGPUDevice device{};
    {
        WGPUDeviceDescriptor desc{};
        device = request_device(adapter, &desc);
        if (!device)
        {
            fmt::print("Failed to get WebGPU device\n");
            return 1;
        }

        // Set error callback to provide debug info
        constexpr auto device_error_cb =
            [](WGPUErrorType type, char const* message, void* /*userdata*/)
        {
            fmt::print(
                "Device error: {} ({})\nMessage: {}\n",
                to_string(type),
                static_cast<int>(type),
                message);
        };
        wgpuDeviceSetUncapturedErrorCallback(device, device_error_cb, nullptr);
    }
    auto const drop_device = defer([=]() { wgpuDeviceRelease(device); });

    // Get the device's queue
    WGPUQueue const queue = wgpuDeviceGetQueue(device);
    {
        // Add callback which fires whenever queued work is done
        auto const work_done_cb = [](WGPUQueueWorkDoneStatus const status, void* /*userdata*/)
        { fmt::print("Queued work finished with status: {}\n", to_string(status)); };

        wgpuQueueOnSubmittedWorkDone(queue, work_done_cb, nullptr);
    }

    // Configure surface via the device (replaces swap chain API)
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        WGPUSurfaceConfiguration config{};
        {
            config.device = device;
            config.width = width;
            config.height = height;
            config.format = wgpuSurfaceGetPreferredFormat(surface, adapter);
            config.usage = WGPUTextureUsage_RenderAttachment;
            config.presentMode = WGPUPresentMode_Mailbox;
        }
        wgpuSurfaceConfigure(surface, &config);
    }
    auto const unconfig_srf = defer([&] { wgpuSurfaceUnconfigure(surface); });

    // Create shader module
    WGPUShaderModule const shader = make_shader_module(device, shader_src);
    auto const drop_shader = defer([&]() { wgpuShaderModuleRelease(shader); });

    // Create render pipeline
    WGPURenderPipeline const pipeline =
        make_render_pipeline(device, shader, wgpuSurfaceGetPreferredFormat(surface, adapter));
    auto const drop_pipeline = defer([&]() { wgpuRenderPipelineRelease(pipeline); });

    // Start frame loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Get the texture to render the frame to
        WGPUSurfaceTexture curr_tex;
        wgpuSurfaceGetCurrentTexture(surface, &curr_tex);
        if (curr_tex.status != WGPUSurfaceGetCurrentTextureStatus_Success)
        {
            fmt::print("Failed to get surface texture ({})\n", to_string(curr_tex.status));
            return 1;
        }

        // Create a command encoder from the device
        WGPUCommandEncoder encoder{};
        {
            WGPUCommandEncoderDescriptor desc{};
            encoder = wgpuDeviceCreateCommandEncoder(device, &desc);
        }
        auto const drop_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // Can use debug markers on the encoder for printf-like debugging bw commands
        // wgpuCommandEncoderInsertDebugMarker(encoder, "Do a thing");
        // wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

        // Render pass
        {
            WGPUTextureView const tex_view = make_texture_view(curr_tex);
            // TODO(dr): Example releases the texture view after render pass closed
            auto const drop_tex_view = defer([&]() { wgpuTextureViewRelease(tex_view); });

            WGPURenderPassEncoder const pass = begin_render_pass(encoder, tex_view);
            auto const end_pass = defer([&]() { wgpuRenderPassEncoderEnd(pass); });

            wgpuRenderPassEncoderSetPipeline(pass, pipeline);
            wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);
        }

        // Create a command via the encoder
        WGPUCommandBuffer command{};
        {
            WGPUCommandBufferDescriptor desc{};
            command = wgpuCommandEncoderFinish(encoder, &desc);
        }

        // Submit encoded command
        wgpuQueueSubmit(queue, 1, &command);

        // Display the result
        wgpuSurfacePresent(surface);
    }

    return 0;
}
