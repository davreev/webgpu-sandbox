#include <cassert>

#include <fmt/core.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <webgpu/webgpu.h>

#include <dr/basic_types.hpp>
#include <dr/container_utils.hpp>
#include <dr/defer.hpp>
#include <dr/linalg_reshape.hpp>
#include <dr/math.hpp>
#include <dr/math_types.hpp>
#include <dr/memory.hpp>
#include <dr/span.hpp>

#include <wgpu_utils.hpp>

#include "assets.hpp"
#include "graphics.h"

#include "../dr_shim.hpp"
#include "../gfx_utils.hpp"

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
        result.surface = make_surface(result.instance, "#textured-mesh");
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

        // Provide uncaptured error callback to device creation
        WGPUDeviceDescriptor device_desc = {};
        device_desc.uncapturedErrorCallbackInfo.callback = //
            [](WGPUDevice const* /*device*/,
               WGPUErrorType type,
               WGPUStringView msg,
               void* /*userdata1*/,
               void* /*userdata2*/) {
                fmt::print(
                    "WebGPU device error: {} ({})\nMessage: {}\n",
                    to_string(type),
                    int(type),
                    msg.data);
            };

        // Create WebGPU device
        result.device = request_device(result.instance, result.adapter, &device_desc);
        assert(result.device);

        result.surface_format = WGPUTextureFormat_BGRA8Unorm;
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

struct RenderPass
{
    WGPURenderPassEncoder encoder;
    WGPUTextureView surface_view;

    static RenderPass begin(
        WGPUCommandEncoder const cmd_encoder,
        WGPUSurface const surface,
        WGPUTextureView const depth)
    {
        RenderPass result{};

        result.surface_view = surface_make_view(surface);
        assert(result.surface_view);

        result.encoder = render_pass_begin(cmd_encoder, result.surface_view, depth);
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

struct DepthTarget
{
    static constexpr WGPUTextureFormat format = WGPUTextureFormat_Depth32Float;
    WGPUTexture texture;
    WGPUTextureView view;

    static DepthTarget make(WGPUDevice const device, i32 const width, i32 const height)
    {
        DepthTarget result{};
        result.texture = depth_target_make_texture(device, width, height, format);
        result.view = depth_target_make_view(result.texture);
        return result;
    }

    static void release(DepthTarget& target)
    {
        wgpuTextureViewRelease(target.view);
        wgpuTextureRelease(target.texture);
        target = {};
    }

    void resize(WGPUDevice const device, i32 const width, i32 const height)
    {
        release(*this);
        *this = make(device, width, height);
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

    static RenderMesh make_box(WGPUDevice const device)
    {
        // clang-format off
        // Format: x, y, z, u, v
        static constexpr f32 vertices[][5]{
            // -x
            {0.0, 0.0, 0.0, 0.25, 1.0},
            {0.0, 1.0, 0.0, 0.0, 1.0},
            {0.0, 0.0, 1.0, 0.25, 0.5},
            {0.0, 1.0, 1.0, 0.0, 0.5},
            // +x
            {1.0, 0.0, 0.0, 0.25, 1.0},
            {1.0, 1.0, 0.0, 0.5, 1.0},
            {1.0, 0.0, 1.0, 0.25, 0.5},
            {1.0, 1.0, 1.0, 0.5, 0.5},
            // -y
            {0.0, 0.0, 0.0, 0.5, 1.0},
            {1.0, 0.0, 0.0, 0.75, 1.0},
            {0.0, 0.0, 1.0, 0.5, 0.5},
            {1.0, 0.0, 1.0, 0.75, 0.5},
            // +y
            {0.0, 1.0, 0.0, 1.0, 1.0},
            {1.0, 1.0, 0.0, 0.75, 1.0},
            {0.0, 1.0, 1.0, 1.0, 0.5},
            {1.0, 1.0, 1.0, 0.75, 0.5},
            // -z
            {0.0, 0.0, 0.0, 0.25, 0.5},
            {1.0, 0.0, 0.0, 0.0, 0.5},
            {0.0, 1.0, 0.0, 0.25, 0.0},
            {1.0, 1.0, 0.0, 0.0, 0.0},
            // +z
            {0.0, 0.0, 1.0, 0.25, 0.5},
            {1.0, 0.0, 1.0, 0.5, 0.5},
            {0.0, 1.0, 1.0, 0.25, 0.0},
            {1.0, 1.0, 1.0, 0.5, 0.0},
        };
        // clang-format on

        static constexpr u16 faces[][3]{
            {1, 0, 2},
            {2, 3, 1},
            {4, 5, 7},
            {7, 6, 4},

            {8, 9, 11},
            {11, 10, 8},
            {13, 12, 14},
            {14, 15, 13},

            {17, 16, 18},
            {18, 19, 17},
            {20, 21, 23},
            {23, 22, 20},
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

struct RenderMaterial
{
    static inline WGPUBindGroupLayout bind_group_layout{};
    static inline WGPUPipelineLayout pipeline_layout{};
    static inline WGPURenderPipeline pipeline{};
    struct
    {
        struct
        {
            WGPUTexture texture;
            WGPUTextureView view;
            WGPUSampler sampler;
        } color_map;
    } static inline defaults{};

    struct Instance
    {
        struct
        {
            f32 local_to_clip[16];
        } uniforms{};

        WGPUBuffer uniform_buffer;
        WGPUBindGroup bind_group;

        static Instance make(WGPUDevice const device)
        {
            Instance result{};

            result.uniform_buffer = render_material_make_uniform_buffer(device, sizeof(uniforms));
            assert(result.uniform_buffer);

            result.update_bind_group(device);
            return result;
        }

        static void release(Instance& instance)
        {
            wgpuBufferRelease(instance.uniform_buffer);
            wgpuBindGroupRelease(instance.bind_group);
            instance = {};
        }

        void update_bind_group(WGPUDevice const device)
        {
            if (bind_group)
                wgpuBindGroupRelease(bind_group);
            bind_group = render_material_make_bind_group(
                device,
                bind_group_layout,
                defaults.color_map.view,
                defaults.color_map.sampler,
                uniform_buffer);
            assert(bind_group);
        }

        void update_uniform_buffer(WGPUQueue const queue)
        {
            wgpuQueueWriteBuffer(queue, uniform_buffer, 0, &uniforms, sizeof(uniforms));
        }

        void apply_pipeline(WGPURenderPassEncoder const encoder)
        {
            wgpuRenderPassEncoderSetPipeline(encoder, pipeline);
        }

        void bind_resources(WGPURenderPassEncoder const encoder)
        {
            wgpuRenderPassEncoderSetBindGroup(encoder, 0, bind_group, 0, nullptr);
        }
    };

    static void init(WGPUDevice const device, WGPUTextureFormat const surface_format)
    {
        bind_group_layout = render_material_make_bind_group_layout(device);
        pipeline_layout = render_material_make_pipeline_layout(device, bind_group_layout);

        // Init pipeline
        {
            ShaderAsset const& asset = load_shader_asset("assets/shaders/unlit_texture.wgsl");
            pipeline = render_material_make_pipeline(
                device,
                pipeline_layout,
                {asset.src.c_str(), WGPU_STRLEN},
                surface_format,
                DepthTarget::format);
            assert(pipeline);
        }

        // Init default resources
        {
            auto& color_map = defaults.color_map;
            ImageAsset const& asset = load_image_asset("assets/images/cube-faces.png");
            color_map.texture = render_material_make_color_texture(
                device,
                asset.data.get(),
                asset.width,
                asset.height,
                asset.stride,
                &color_map.view,
                &color_map.sampler);
            assert(color_map.texture);
            assert(color_map.view);
            assert(color_map.sampler);
        }
    }

    static void deinit()
    {
        wgpuTextureViewRelease(defaults.color_map.view);
        wgpuSamplerRelease(defaults.color_map.sampler);
        wgpuTextureRelease(defaults.color_map.texture);
        defaults = {};

        wgpuRenderPipelineRelease(pipeline);
        pipeline = {};

        wgpuPipelineLayoutRelease(pipeline_layout);
        pipeline_layout = {};

        wgpuBindGroupLayoutRelease(bind_group_layout);
        bind_group_layout = {};
    }
};

struct AppState
{
    GLFWwindow* window;
    GpuContext gpu;
    DepthTarget depth;
    RenderMaterial::Instance material;
    RenderMesh geometry;
    struct
    {
        f32 fov_y{deg_to_rad(60.0f)};
        f32 clip_near{0.01f};
        f32 clip_far{100.0f};
    } view;
    usize frame_count;
};

AppState state{};

void init_app()
{
    // Initialize GLFW
    bool const glfw_ok = glfwInit();
    assert(glfw_ok);

    // Create GLFW window
#ifdef __EMSCRIPTEN__
    int init_width, init_height;
    get_canvas_client_size(init_width, init_height);
#else
    constexpr int init_width = 800;
    constexpr int init_height = 600;
#endif
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    state.window = glfwCreateWindow(
        init_width,
        init_height,
        "WebGPU Sandbox: Textured Mesh",
        nullptr,
        nullptr);
    assert(state.window);

    // Create WebGPU context
    state.gpu = GpuContext::make(state.window);
    state.depth = DepthTarget::make(state.gpu.device, init_width, init_height);

#ifdef __EMSCRIPTEN__
    // Handle canvas resize
    auto constexpr resize_cb =
        [](int /*event_type*/, EmscriptenUiEvent const* /*event*/, void* /*userdata*/) -> bool {
        int w, h;
        get_canvas_client_size(w, h);
        glfwSetWindowSize(state.window, w, h);
        return true;
    };
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, false, resize_cb);

    // Handle framebuffer resize
    glfwSetFramebufferSizeCallback(state.window, [](GLFWwindow* /*window*/, int width, int height) {
        state.depth.resize(state.gpu.device, width, height);
    });
#else
    // Handle framebuffer resize
    glfwSetFramebufferSizeCallback(state.window, [](GLFWwindow* /*window*/, int width, int height) {
        state.gpu.config_surface(width, height);
        state.depth.resize(state.gpu.device, width, height);
    });
#endif

    // Init materials and create instance
    RenderMaterial::init(state.gpu.device, state.gpu.surface_format);
    state.material = RenderMaterial::Instance::make(state.gpu.device);

    // Create mesh
    state.geometry = RenderMesh::make_box(state.gpu.device);
}

void deinit_app()
{
    RenderMesh::release(state.geometry);
    RenderMaterial::Instance::release(state.material);
    DepthTarget::release(state.depth);
    GpuContext::release(state.gpu);
    glfwDestroyWindow(state.window);
    glfwTerminate();
    state = {};
}

f32 get_window_aspect()
{
    int w, h;
    glfwGetWindowSize(state.window, &w, &h);
    return static_cast<f32>(w) / h;
}

Mat4<f32> make_local_to_world()
{
    constexpr f64 turns_per_frame = pi<f64> * 0.004;
    Mat3<f32> r = Mat3<f32>::Identity();
    r.topLeftCorner<2, 2>() = make_rotate(state.frame_count * turns_per_frame);
    return make_affine(r, r * vec<3>(-0.5f));
}

Vec3<f32> get_camera_position()
{
    constexpr f64 cycles_per_frame = 0.002;
    constexpr f64 spread = 0.3 * pi<f64>;
    f32 const t = spread * std::sin(state.frame_count * (cycles_per_frame * 2.0 * pi<f64>));

    constexpr f32 rad = 3.0f;
    return {0.0, rad * std::cos(t), rad * std::sin(t)};
}

Mat4<f32> make_world_to_view()
{
    return make_look_at(get_camera_position(), vec<3>(0.0f), vec(0.0f, 0.0f, 1.0f));
}

Mat4<f32> make_view_to_clip()
{
    return make_perspective(
        state.view.fov_y,
        get_window_aspect(),
        state.view.clip_near,
        state.view.clip_far);
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

        WGPUQueue const queue = wgpuDeviceGetQueue(state.gpu.device);

        // Render pass
        {
            RenderPass pass = RenderPass::begin(cmd_encoder, state.gpu.surface, state.depth.view);
            auto const end_pass = defer([&]() { RenderPass::end(pass); });

            auto& mat = state.material;
            mat.apply_pipeline(pass.encoder);

            Mat4<f32> const local_to_world = make_local_to_world();
            Mat4<f32> const world_to_view = make_world_to_view();
            Mat4<f32> const view_to_clip = make_view_to_clip();

            as_mat<4, 4>(mat.uniforms.local_to_clip) = //
                view_to_clip * world_to_view * local_to_world;

            mat.update_uniform_buffer(queue);
            mat.bind_resources(pass.encoder);

            auto& geom = state.geometry;
            geom.bind_resources(pass.encoder);
            geom.dispatch_draw(pass.encoder);
        }

        // Create encoded commands
        WGPUCommandBuffer const cmds = wgpuCommandEncoderFinish(cmd_encoder, nullptr);
        assert(cmds);
        auto const drop_cmds = defer([=]() { wgpuCommandBufferRelease(cmds); });

        // Submit encoded commands
        wgpuQueueSubmit(queue, 1, &cmds);

        ++state.frame_count;
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
