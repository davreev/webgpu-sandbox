#include <cassert>
#include <cstdint>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include <webgpu/app.hpp>
#include <webgpu/glfw.h>

#include "shader_src.hpp"
#include "wgpu_config.h"

namespace wgpu
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

GpuContext make_gpu_context(GLFWwindow* window)
{
    GpuContext result{};

    // Create WebGPU instance
    result.instance = wgpuCreateInstance(nullptr);
    if (!result.instance)
    {
        fmt::print("Failed to create WebGPU instance\n");
        return result;
    }

    // Get WebGPU surface from GLFW window
    result.surface = glfwGetWGPUSurface(result.instance, window);
    if (!result.surface)
    {
        fmt::print("Failed to get WebGPU surface\n");
        return result;
    }

    // Create WGPU adapter
    WGPURequestAdapterOptions options{};
    {
        options.compatibleSurface = result.surface;
        options.powerPreference = WGPUPowerPreference_HighPerformance;
    }
    result.adapter = request_adapter(result.instance, &options);
    if (!result.adapter)
    {
        fmt::print("Failed to get WebGPU adapter\n");
        return result;
    }

    // Create WebGPU device
    result.device = request_device(result.adapter);
    if (!result.device)
    {
        fmt::print("Failed to get WebGPU device\n");
        return result;
    }

    // Set error callback on device
    wgpuDeviceSetUncapturedErrorCallback(
        result.device,
        [](WGPUErrorType type, char const* message, void* /*userdata*/) {
            fmt::print(
                "Device error: {} ({})\nMessage: {}\n",
                to_string(type),
                static_cast<int>(type),
                message);
        },
        nullptr);

    // Configure surface (replaces swap chain API)
    // result.surface_format = wgpuSurfaceGetPreferredFormat(result.surface, result.adapter);
    result.surface_format = WGPUTextureFormat_BGRA8Unorm;
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        WGPUSurfaceConfiguration config{};
        {
            config.device = result.device;
            config.width = width;
            config.height = height;
            config.format = result.surface_format;
            config.usage = WGPUTextureUsage_RenderAttachment;
        }

        wgpuSurfaceConfigure(result.surface, &config);
    }

    result.is_valid = true;
    return result;
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

struct Mesh
{
    struct
    {
        WGPUBuffer buffer;
        std::size_t size;
    } vertex;

    struct
    {
        WGPUBuffer buffer;
        std::size_t size;
        WGPUIndexFormat format;
    } index;

    struct
    {
        std::size_t index_start;
        std::size_t index_count;
        std::size_t base_vertex;
    } part;

    bool is_valid;
};

Mesh make_mesh(WGPUDevice const device)
{
    Mesh result{};

    // clang-format off
    static constexpr float vertices[]{
        -0.5, -0.5, 0.0, 0.0,
        0.5, -0.5, 1.0, 0.0,
        -0.5, 0.5, 0.0, 1.0,
        0.5, 0.5, 1.0, 1.0,
    };
    static constexpr std::uint16_t indices[]{
        0, 1, 2,
        3, 2, 1,
    };
    // clang-format on

    result.vertex.buffer =
        make_buffer(device, sizeof(vertices), WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst);
    if(!result.vertex.buffer)
    {
        fmt::print("Failed to create index buffer\n");
        return result;
    }
    result.vertex.size = sizeof(vertices);

    result.index.buffer =
        make_buffer(device, sizeof(indices), WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst);
    if(!result.index.buffer)
    {
        fmt::print("Failed to create index buffer\n");
        return result;
    }
    result.index.size = sizeof(indices);
    result.index.format = WGPUIndexFormat_Uint16;
    result.part.index_count = sizeof(indices) / sizeof(indices[0]);

    // Write data to buffers
    WGPUQueue const queue = wgpuDeviceGetQueue(device);
    wgpuQueueWriteBuffer(queue, result.vertex.buffer, 0, vertices, result.vertex.size);
    wgpuQueueWriteBuffer(queue, result.index.buffer, 0, indices, result.index.size);

    result.is_valid = true;
    return result;
}

void release_mesh(Mesh& mesh)
{
    wgpuBufferRelease(mesh.vertex.buffer);
    wgpuBufferRelease(mesh.index.buffer);
    mesh = {};
}

struct RenderPass
{
    WGPUTextureView view;
    WGPURenderPassEncoder encoder;
    bool is_valid;
};

RenderPass begin_render_pass(WGPUSurface const surface, WGPUCommandEncoder const encoder)
{
    RenderPass result{};

    WGPUSurfaceTexture const srf_tex = get_current_texture(surface);
    if (srf_tex.status != WGPUSurfaceGetCurrentTextureStatus_Success)
    {
        fmt::print("Failed to get surface texture ({})\n", to_string(srf_tex.status));
        return result;
    }

    result.view = make_texture_view(srf_tex);
    if (!result.view)
    {
        fmt::print("Failed to create view of surface texture\n");
        return result;
    }

    result.encoder = begin_render_pass(encoder, result.view);
    if (!result.view)
    {
        fmt::print("Failed to create render pass encoder\n");
        return result;
    }

    result.is_valid = true;
    return result;
}

void end_render_pass(RenderPass& pass)
{
    wgpuRenderPassEncoderEnd(pass.encoder);
    wgpuTextureViewRelease(pass.view);
    pass = {};
}

WGPURenderPipeline make_render_pipeline(
    WGPUDevice const device,
    char const* const shader_src,
    WGPUTextureFormat const surface_format)
{
    // Create shader module
    WGPUShaderModule const shader = make_shader_module(device, shader_src);
    if (!shader)
    {
        fmt::print("Failed to create shader module\n");
        return nullptr;
    }
    auto const drop_shader = defer([=]() { wgpuShaderModuleRelease(shader); });

    // NOTE(dr): Shader module can be released once the pipeline is created
    return make_render_pipeline(device, shader, surface_format);
}

} // namespace wgpu

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

    // Create WebGPU context
    GpuContext ctx = make_gpu_context(window);
    if (!ctx.is_valid)
    {
        fmt::print("Failed to initialize WebGPU context\n");
        return 1;
    }
    auto const drop_ctx = defer([&]() { release_gpu_context(ctx); });

    // Create render pipeline
    WGPURenderPipeline const pipeline =
        make_render_pipeline(ctx.device, hello_mesh::shader_src, ctx.surface_format);
    if(!pipeline)
    {
        fmt::print("Failed to create render pipeline\n");
        return 1;
    }
    auto const drop_pipeline = defer([=]() { wgpuRenderPipelineRelease(pipeline); });

    // Create mesh
    Mesh mesh = make_mesh(ctx.device);
    if(!mesh.is_valid)
    {
        fmt::print("Failed to create mesh\n");
        return 1;
    }
    auto const drop_mesh = defer([&]() { release_mesh(mesh); });

    // Get the device's default queue
    WGPUQueue const queue = wgpuDeviceGetQueue(ctx.device);

    // Frame loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Create a command encoder from the device
        WGPUCommandEncoder const encoder = wgpuDeviceCreateCommandEncoder(ctx.device, nullptr);
        auto const drop_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // Render pass
        {
            RenderPass pass = begin_render_pass(ctx.surface, encoder);
            if (!pass.is_valid)
            {
                fmt::print("Failed to begin render pass\n");
                return 1;
            }
            auto const end_pass = defer([&]() { end_render_pass(pass); });

            wgpuRenderPassEncoderSetPipeline(pass.encoder, pipeline);

            // Draw mesh
            {
                wgpuRenderPassEncoderSetVertexBuffer(
                    pass.encoder,
                    0,
                    mesh.vertex.buffer,
                    0,
                    mesh.vertex.size);

                wgpuRenderPassEncoderSetIndexBuffer(
                    pass.encoder,
                    mesh.index.buffer,
                    mesh.index.format,
                    0,
                    mesh.index.size);

                wgpuRenderPassEncoderDrawIndexed(
                    pass.encoder,
                    mesh.part.index_count,
                    1,
                    mesh.part.index_start,
                    mesh.part.base_vertex,
                    0);
            }
        }

        // Create a command via the encoder
        WGPUCommandBuffer const command = wgpuCommandEncoderFinish(encoder, nullptr);
        auto const drop_command = defer([=]() { wgpuCommandBufferRelease(command); });

        // Submit encoded command
        wgpuQueueSubmit(queue, 1, &command);

        // Present the render target
        wgpuSurfacePresent(ctx.surface);
    }

    return 0;
}
