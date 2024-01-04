#include <cassert>

#include <cstdint>
#include <fmt/core.h>

#include <webgpu/webgpu.h>

#include <webgpu/app.hpp>
#include <webgpu/glfw.h>

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
};

Mesh make_mesh(WGPUDevice const device)
{
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

    WGPUBuffer const vertex_buf =
        make_buffer(device, sizeof(vertices), WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst);

    WGPUBuffer const index_buf =
        make_buffer(device, sizeof(indices), WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst);

    WGPUQueue const queue = wgpuDeviceGetQueue(device);
    wgpuQueueWriteBuffer(queue, vertex_buf, 0, vertices, sizeof(vertices));
    wgpuQueueWriteBuffer(queue, index_buf, 0, indices, sizeof(indices));

    return Mesh{
        {vertex_buf, sizeof(vertices)},
        {index_buf, sizeof(indices), WGPUIndexFormat_Uint16},
        {0, sizeof(indices) / sizeof(std::uint16_t), 0},
    };
}

void release_mesh(Mesh& mesh)
{
    wgpuBufferRelease(mesh.vertex.buffer);
    wgpuBufferRelease(mesh.index.buffer);
    mesh = {};
}

constexpr char const* shader_src = R"(
struct VertexIn {
    @location(0) position: vec2f,
    @location(1) tex_coord: vec2f
};

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) tex_coord: vec2f
};

@vertex
fn vs_main(in : VertexIn) -> VertexOut {
    return VertexOut(vec4f(in.position, 0.0, 1.0), in.tex_coord);
}

struct FragmentIn {
    @location(0) tex_coord: vec2f
};

@fragment
fn fs_main(in : FragmentIn) -> @location(0) vec4f {
    return vec4f(in.tex_coord, 0.5, 1.0);
}
)";

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

    // Create shader module
    WGPUShaderModule const shader = make_shader_module(ctx.device, shader_src);
    auto const drop_shader = defer([=]() { wgpuShaderModuleRelease(shader); });

    // Create render pipeline
    WGPURenderPipeline const pipeline =
        make_render_pipeline(ctx.device, shader, ctx.surface_format);
    auto const drop_pipeline = defer([=]() { wgpuRenderPipelineRelease(pipeline); });

    // Create mesh
    Mesh mesh = make_mesh(ctx.device);
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
            WGPUSurfaceTexture const srf_tex = get_current_texture(ctx.surface);
            if (srf_tex.status != WGPUSurfaceGetCurrentTextureStatus_Success)
            {
                fmt::print("Failed to get surface texture ({})\n", to_string(srf_tex.status));
                return 1;
            }

            WGPUTextureView const tex_view = make_texture_view(srf_tex);
            auto const drop_tex_view = defer([=]() { wgpuTextureViewRelease(tex_view); });

            WGPURenderPassEncoder const pass = begin_render_pass(encoder, tex_view);
            auto const end_pass = defer([=]() { wgpuRenderPassEncoderEnd(pass); });

            wgpuRenderPassEncoderSetPipeline(pass, pipeline);

            // Draw mesh
            {
                wgpuRenderPassEncoderSetVertexBuffer(
                    pass,
                    0,
                    mesh.vertex.buffer,
                    0,
                    mesh.vertex.size);

                wgpuRenderPassEncoderSetIndexBuffer(
                    pass,
                    mesh.index.buffer,
                    mesh.index.format,
                    0,
                    mesh.index.size);

                wgpuRenderPassEncoderDrawIndexed(
                    pass,
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
