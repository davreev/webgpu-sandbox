#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include <wgpu_utils.hpp>

#include "wgpu_config.h"

using namespace wgpu;

namespace
{

struct GpuContext
{
    WGPUInstance instance;
    WGPUSurface surface;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUTextureFormat surface_format;
    bool is_valid;
};

void config_surface(GpuContext& ctx, int width, int height)
{
    WGPUSurfaceConfiguration config{};
    {
        config.device = ctx.device;
        config.width = width;
        config.height = height;
        config.format = get_preferred_texture_format(ctx.surface, ctx.adapter);
        config.usage = WGPUTextureUsage_RenderAttachment;
    }

    ctx.surface_format = config.format; // Cache surface format
    wgpuSurfaceConfigure(ctx.surface, &config);
}

GpuContext make_gpu_context(GLFWwindow* window)
{
    GpuContext ctx{};

    // Create WebGPU instance
    ctx.instance = wgpuCreateInstance(nullptr);
    if (!ctx.instance)
    {
        fmt::print("Failed to create WebGPU instance\n");
        return ctx;
    }

    // Get WebGPU surface from GLFW window
    ctx.surface = make_surface(ctx.instance, window);
    if (!ctx.surface)
    {
        fmt::print("Failed to get WebGPU surface\n");
        return ctx;
    }

    // Create WGPU adapter
    WGPURequestAdapterOptions options{};
    {
        options.compatibleSurface = ctx.surface;
        options.powerPreference = WGPUPowerPreference_HighPerformance;
    }
    ctx.adapter = request_adapter(ctx.instance, &options);
    if (!ctx.adapter)
    {
        fmt::print("Failed to get WebGPU adapter\n");
        return ctx;
    }

#if true
    // Report adapter details
    report_features(ctx.adapter);
    report_limits(ctx.adapter);
    report_properties(ctx.adapter);
    report_surface_capabilities(ctx.surface, ctx.adapter);
#endif

    // Create WebGPU device
    ctx.device = request_device(ctx.adapter);
    if (!ctx.device)
    {
        fmt::print("Failed to get WebGPU device\n");
        return ctx;
    }

    // Set error callback on device
    wgpuDeviceSetUncapturedErrorCallback(
        ctx.device,
        [](WGPUErrorType type, char const* msg, void* /*userdata*/) {
            fmt::print(
                "WebGPU device error: {} ({})\nMessage: {}\n",
                to_string(type),
                static_cast<int>(type),
                msg);
        },
        nullptr);

    // Configure surface (replaces swap chain API)
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        config_surface(ctx, width, height);
    }

    ctx.is_valid = true;
    return ctx;
}

void release_gpu_context(GpuContext& ctx)
{
    wgpuSurfaceUnconfigure(ctx.surface);
    wgpuDeviceRelease(ctx.device);
    wgpuAdapterRelease(ctx.adapter);
    wgpuSurfaceRelease(ctx.surface);
    wgpuInstanceRelease(ctx.instance);
    ctx = {};
}

struct RenderPass
{
    WGPUTextureView view;
    WGPURenderPassEncoder encoder;
    bool is_valid;
};

RenderPass begin_render_pass(WGPUSurface const surface, WGPUCommandEncoder const encoder)
{
    RenderPass pass{};

    WGPUSurfaceTexture const srf_tex = get_current_texture(surface);
    if (srf_tex.status != WGPUSurfaceGetCurrentTextureStatus_Success)
    {
        fmt::print("Failed to get surface texture ({})\n", to_string(srf_tex.status));
        return pass;
    }

    pass.view = make_texture_view(srf_tex);
    if (!pass.view)
    {
        fmt::print("Failed to create view of surface texture\n");
        return pass;
    }

    pass.encoder = begin_render_pass(encoder, pass.view);
    if (!pass.view)
    {
        fmt::print("Failed to create render pass encoder\n");
        return pass;
    }

    pass.is_valid = true;
    return pass;
}

void end_render_pass(RenderPass& pass)
{
    wgpuRenderPassEncoderEnd(pass.encoder);
    wgpuTextureViewRelease(pass.view);
    pass = {};
}

} // namespace

int main(int /*argc*/, char** /*argv*/)
{
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
    GLFWwindow* const window =
        glfwCreateWindow(800, 600, "WebGPU Sandbox: Clear Screen", nullptr, nullptr);
    if (!window)
    {
        fmt::print("Failed to create window\n");
        return 1;
    }
    auto const drop_window = defer([=]() { glfwDestroyWindow(window); });

    // Create WebGPU context
    GpuContext ctx = make_gpu_context(window);
    if (!ctx.is_valid)
    {
        fmt::print("Failed to initialize WebGPU context\n");
        return 1;
    }
    auto const drop_ctx = defer([&]() { release_gpu_context(ctx); });

    // Get the device's queue
    WGPUQueue const queue = wgpuDeviceGetQueue(ctx.device);

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
        WGPUCommandEncoder const encoder = wgpuDeviceCreateCommandEncoder(ctx.device, nullptr);
        auto const drop_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // NOTE(dr): Can use debug markers on the encoder for printf-like debugging bw commands
        // wgpuCommandEncoderInsertDebugMarker(encoder, "Do a thing");
        // wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

        // Render pass
        {
            RenderPass pass = begin_render_pass(ctx.surface, encoder);
            if (!pass.is_valid)
            {
                fmt::print("Failed to begin render pass\n");
                return 1;
            }
            auto const end_pass = defer([&]() { end_render_pass(pass); });

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
        wgpuSurfacePresent(ctx.surface);
    }

    return 0;
}
