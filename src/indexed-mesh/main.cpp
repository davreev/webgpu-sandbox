#include <cassert>
#include <cstdint>

#include <fmt/core.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <webgpu/webgpu.h>

#include <wgpu_utils.hpp>

#include "../defer.hpp"
#include "shader_src.hpp"
#include "wgpu_config.h"

namespace wgpu::sandbox
{
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
#ifdef __EMSCRIPTEN__
        // NOTE(dr): Default value from Emscripten's webgpu.h is undefined
        config.presentMode = WGPUPresentMode_Fifo;
#endif
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

#ifdef __EMSCRIPTEN__
    // Get WebGPU surface from the HTML canvas
    ctx.surface = make_surface(ctx.instance, "#indexed-mesh");
#else
    // Get WebGPU surface from GLFW window
    ctx.surface = make_surface(ctx.instance, window);
#endif
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

    // Configure surface
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
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
    if (!result.vertex.buffer)
    {
        fmt::print("Failed to create index buffer\n");
        return result;
    }
    result.vertex.size = sizeof(vertices);

    result.index.buffer =
        make_buffer(device, sizeof(indices), WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst);
    if (!result.index.buffer)
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

struct State
{
    GLFWwindow* window;
    GpuContext gpu;
    WGPURenderPipeline pipeline;
    Mesh mesh;
    WGPUQueue queue;
};

State state{};

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    // Initialize GLFW
    if (!glfwInit())
    {
        fmt::print("Failed to initialize GLFW\n");
        return 1;
    }
    auto const deinit_glfw = defer([]() { glfwTerminate(); });

    // Create GLFW window
#ifdef __EMSCRIPTEN__
    int init_width, init_height;
    wgpu::get_canvas_client_size(init_width, init_height);
#else
    constexpr int init_width = 800;
    constexpr int init_height = 600;
#endif
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    state.window =
        glfwCreateWindow(init_width, init_height, "WebGPU Sandbox: Indexed Mesh", nullptr, nullptr);
    if (!state.window)
    {
        fmt::print("Failed to create window\n");
        return 1;
    }
    auto const drop_window = defer([]() { glfwDestroyWindow(state.window); });

    // Create WebGPU context
    state.gpu = make_gpu_context(state.window);
    if (!state.gpu.is_valid)
    {
        fmt::print("Failed to initialize WebGPU context\n");
        return 1;
    }
    auto const drop_gpu = defer([&]() { release_gpu_context(state.gpu); });

#ifdef __EMSCRIPTEN__
    // Handle canvas resize
    emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,
        nullptr,
        false,
        [](int /*event_type*/, EmscriptenUiEvent const* /*event*/, void* /*userdata*/) -> bool {
            int new_size[2];
            wgpu::get_canvas_client_size(new_size[0], new_size[1]);
            glfwSetWindowSize(state.window, new_size[0], new_size[1]);
            return true;
        });
#else
    // Handle window resize
    glfwSetFramebufferSizeCallback(state.window, [](GLFWwindow* /*window*/, int width, int height) {
        config_surface(state.gpu, width, height);
    });
#endif

    // Create render pipeline
    state.pipeline =
        make_render_pipeline(state.gpu.device, shader_src, state.gpu.surface_format);
    if (!state.pipeline)
    {
        fmt::print("Failed to create render pipeline\n");
        return 1;
    }
    auto const drop_pipeline = defer([=]() { wgpuRenderPipelineRelease(state.pipeline); });

    // Create mesh
    state.mesh = make_mesh(state.gpu.device);
    if (!state.mesh.is_valid)
    {
        fmt::print("Failed to create mesh\n");
        return 1;
    }
    auto const drop_mesh = defer([&]() { release_mesh(state.mesh); });

    // Cache the device's default queue
    state.queue = wgpuDeviceGetQueue(state.gpu.device);

    // Main loop body
    constexpr auto loop_body = []() {
        glfwPollEvents();

        // Create a command encoder from the device
        WGPUCommandEncoder const encoder =
            wgpuDeviceCreateCommandEncoder(state.gpu.device, nullptr);
        auto const drop_encoder = defer([=]() { wgpuCommandEncoderRelease(encoder); });

        // Render pass
        {
            RenderPass pass = begin_render_pass(state.gpu.surface, encoder);
            assert(pass.is_valid);
            auto const end_pass = defer([&]() { end_render_pass(pass); });

            wgpuRenderPassEncoderSetPipeline(pass.encoder, state.pipeline);

            // Draw mesh
            {
                wgpuRenderPassEncoderSetVertexBuffer(
                    pass.encoder,
                    0,
                    state.mesh.vertex.buffer,
                    0,
                    state.mesh.vertex.size);

                wgpuRenderPassEncoderSetIndexBuffer(
                    pass.encoder,
                    state.mesh.index.buffer,
                    state.mesh.index.format,
                    0,
                    state.mesh.index.size);

                wgpuRenderPassEncoderDrawIndexed(
                    pass.encoder,
                    state.mesh.part.index_count,
                    1,
                    state.mesh.part.index_start,
                    state.mesh.part.base_vertex,
                    0);
            }
        }

        // Create encoded commands
        WGPUCommandBuffer const commands = wgpuCommandEncoderFinish(encoder, nullptr);
        auto const drop_command = defer([=]() { wgpuCommandBufferRelease(commands); });

        // Submit encoded commands
        wgpuQueueSubmit(state.queue, 1, &commands);
    };

    // Main loop
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop_body, 0, true);
#else
    while (!glfwWindowShouldClose(state.window))
    {
        loop_body();
        wgpuSurfacePresent(state.gpu.surface);
    }
#endif

    return 0;
}
