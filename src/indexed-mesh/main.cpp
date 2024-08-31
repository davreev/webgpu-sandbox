#include <cassert>
#include <cstdint>

#include <fmt/core.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <webgpu/webgpu.h>

#include <dr/basic_types.hpp>
#include <dr/container_utils.hpp>
#include <dr/defer.hpp>
#include <dr/memory.hpp>
#include <dr/span.hpp>

#include <wgpu_utils.hpp>

#include "../shared/dr_shim.hpp"
#include "graphics.h"
#include "shader_src.hpp"

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

    static GpuContext make(GLFWwindow* const window)
    {
        GpuContext result{};

        // Create WebGPU instance
        result.instance = wgpuCreateInstance(nullptr);
        assert(result.instance);

#ifdef __EMSCRIPTEN__
        // Get WebGPU surface from the HTML canvas
        result.surface = make_surface(result.instance, "#indexed-mesh");
#else
        // Get WebGPU surface from GLFW window
        result.surface = make_surface(result.instance, window);
#endif
        assert(result.surface);

        // Create WGPU adapter
        WGPURequestAdapterOptions options{};
        {
            options.compatibleSurface = result.surface;
            options.powerPreference = WGPUPowerPreference_HighPerformance;
        }
        result.adapter = request_adapter(result.instance, &options);
        assert(result.adapter);

        // Create WebGPU device
        result.device = request_device(result.adapter);
        assert(result.device);

        // Set error callback on device
        wgpuDeviceSetUncapturedErrorCallback(
            result.device,
            [](WGPUErrorType type, char const* msg, void* /*userdata*/) {
                fmt::print(
                    "WebGPU device error: {} ({})\nMessage: {}\n",
                    to_string(type),
                    static_cast<int>(type),
                    msg);
            },
            nullptr);

        result.surface_format = get_preferred_texture_format(result.surface, result.adapter);
        result.config_surface(window);

        return result;
    }

    static void release(GpuContext& ctx)
    {
        wgpuSurfaceUnconfigure(ctx.surface);
        wgpuDeviceRelease(ctx.device);
        wgpuAdapterRelease(ctx.adapter);
        wgpuSurfaceRelease(ctx.surface);
        wgpuInstanceRelease(ctx.instance);
        ctx = {};
    }

    void config_surface(int const width, int const height)
    {
        WGPUSurfaceConfiguration config{};
        {
            config.device = device;
            config.width = width;
            config.height = height;
            config.format = surface_format;
            config.usage = WGPUTextureUsage_RenderAttachment;
#ifdef __EMSCRIPTEN__
            // NOTE(dr): Default value from Emscripten's webgpu.h is undefined
            config.presentMode = WGPUPresentMode_Fifo;
#endif
        }
        wgpuSurfaceConfigure(surface, &config);
    }

    void config_surface(GLFWwindow* const window)
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        config_surface(width, height);
    }
};

struct RenderMesh
{
    static constexpr WGPUIndexFormat index_format{WGPUIndexFormat_Uint16};
    WGPUBuffer vertices;
    WGPUBuffer indices;
    isize index_count;

    static RenderMesh make(
        WGPUDevice const device,
        Span<u8 const> const& vertex_data,
        Span<u8 const> const& index_data)
    {
        RenderMesh result{};

        // Create buffers
        result.vertices = render_mesh_make_buffer(
            device,
            vertex_data.size(),
            WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst);
        assert(result.vertices);

        result.indices = render_mesh_make_buffer(
            device,
            index_data.size(),
            WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst);
        assert(result.indices);

        auto const unmap = defer([&]() {
            wgpuBufferUnmap(result.vertices);
            wgpuBufferUnmap(result.indices);
        });

        // Copy data to buffers
        auto const copy_data = [](WGPUBuffer const dst, Span<u8 const> const& src) {
            void* dst_ptr = wgpuBufferGetMappedRange(dst, 0, src.size());
            assert(dst_ptr);
            std::memcpy(dst_ptr, src.data(), src.size());
        };
        copy_data(result.vertices, vertex_data);
        copy_data(result.indices, index_data);

        constexpr i8 index_stride = sizeof(u16);
        result.index_count = index_data.size() / index_stride;

        return result;
    }

    static RenderMesh make_quad(WGPUDevice const device)
    {
        // Format: x, y, z, u, v
        static constexpr f32 vertices[][4]{
            {-0.5, -0.5, 0.0, 0.0},
            {0.5, -0.5, 1.0, 0.0},
            {-0.5, 0.5, 0.0, 1.0},
            {0.5, 0.5, 1.0, 1.0},
        };

        static constexpr u16 faces[][3]{
            {0, 1, 2},
            {3, 2, 1},
        };

        return make(device, as<u8>(as_span(vertices)), as<u8>(as_span(faces)));
    }

    static void release(RenderMesh& mesh)
    {
        wgpuBufferRelease(mesh.vertices);
        wgpuBufferRelease(mesh.indices);
        mesh = {};
    }

    void bind_resources(WGPURenderPassEncoder const encoder)
    {
        wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, vertices, 0, wgpuBufferGetSize(vertices));
        wgpuRenderPassEncoderSetIndexBuffer(
            encoder,
            indices,
            index_format,
            0,
            wgpuBufferGetSize(indices));
    }

    void dispatch_draw(WGPURenderPassEncoder const encoder) const
    {
        wgpuRenderPassEncoderDrawIndexed(encoder, index_count, 1, 0, 0, 0);
    }
};

struct RenderPass
{
    WGPURenderPassEncoder encoder;
    WGPUTextureView surface_view;

    static RenderPass begin(WGPUSurface const surface, WGPUCommandEncoder const encoder)
    {
        RenderPass result{};

        result.surface_view = surface_make_view(surface);
        assert(result.surface_view);

        result.encoder = render_pass_begin(encoder, result.surface_view);
        assert(result.encoder);

        return result;
    }

    static void end(RenderPass& pass)
    {
        wgpuRenderPassEncoderEnd(pass.encoder);
        wgpuTextureViewRelease(pass.surface_view);
        pass = {};
    }
};

struct AppState
{
    GLFWwindow* window;
    GpuContext gpu;
    WGPURenderPipeline pipeline;
    RenderMesh geometry;
};

AppState state{};

void init_app()
{
    glfwSetErrorCallback(
        [](int errc, char const* msg) { fmt::print("GLFW error: {}\nMessage: {}\n", errc, msg); });

    // Initialize GLFW
    bool const glfw_ok = glfwInit();
    assert(glfw_ok);

    // Create GLFW window
#ifdef __EMSCRIPTEN__
    int init_width, init_height;
    wgpu::get_canvas_client_size(init_width, init_height);
#else
    constexpr int init_width = 800;
    constexpr int init_height = 600;
#endif
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    state.window = glfwCreateWindow(
        init_width,
        init_height,
        "WebGPU Sandbox: Indexed Mesh",
        nullptr,
        nullptr);
    assert(state.window);

    // Create WebGPU context
    state.gpu = GpuContext::make(state.window);

#ifdef __EMSCRIPTEN__
    // Handle canvas resize
    auto constexpr resize_cb =
        [](int /*event_type*/, EmscriptenUiEvent const* /*event*/, void* /*userdata*/) -> bool {
        int w, h;
        wgpu::get_canvas_client_size(w, h);
        glfwSetWindowSize(state.window, w, h);
        return true;
    };
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, false, resize_cb);
#else
    // Handle framebuffer resize
    glfwSetFramebufferSizeCallback(state.window, [](GLFWwindow* /*window*/, int width, int height) {
        state.gpu.config_surface(width, height);
    });
#endif

    // Create render pipeline
    state.pipeline = make_render_pipeline(state.gpu.device, shader_src, state.gpu.surface_format);

    // Create geometry
    state.geometry = RenderMesh::make_quad(state.gpu.device);
}

void deinit_app()
{
    RenderMesh::release(state.geometry);
    wgpuRenderPipelineRelease(state.pipeline);
    GpuContext::release(state.gpu);
    glfwDestroyWindow(state.window);
    glfwTerminate();
    state = {};
}

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    init_app();
    auto const _ = defer([]() { deinit_app(); });

    // Main loop body
    constexpr auto loop_body = []() {
        glfwPollEvents();

        // Create a command encoder from the device
        WGPUCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(
            state.gpu.device,
            nullptr);
        assert(cmd_encoder);
        auto const drop_cmd_encoder = defer([=]() { wgpuCommandEncoderRelease(cmd_encoder); });

        // Render pass
        {
            RenderPass pass = RenderPass::begin(state.gpu.surface, cmd_encoder);
            auto const end_pass = defer([&]() { RenderPass::end(pass); });

            wgpuRenderPassEncoderSetPipeline(pass.encoder, state.pipeline);
            state.geometry.bind_resources(pass.encoder);
            state.geometry.dispatch_draw(pass.encoder);
        }

        // Create encoded commands
        WGPUCommandBuffer const cmds = wgpuCommandEncoderFinish(cmd_encoder, nullptr);
        assert(cmds);
        auto const drop_cmds = defer([=]() { wgpuCommandBufferRelease(cmds); });

        // Submit encoded commands
        WGPUQueue const queue = wgpuDeviceGetQueue(state.gpu.device);
        wgpuQueueSubmit(queue, 1, &cmds);
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
