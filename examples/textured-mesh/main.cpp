#include <cassert>

#include <fmt/core.h>

#include <webgpu/webgpu.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include <dr/basic_types.hpp>
#include <dr/container_utils.hpp>
#include <dr/defer.hpp>
#include <dr/linalg_reshape.hpp>
#include <dr/math.hpp>
#include <dr/math_types.hpp>
#include <dr/memory.hpp>
#include <dr/span.hpp>

#include <dr/app/gfx_utils.hpp>

#include <emsc_utils.hpp>
#include <wgpu_utils.hpp>

#include "assets.hpp"

#include "../example_base.hpp"

namespace wgpu::sandbox
{
namespace
{

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

        result.surface_view = make_view(surface);
        assert(result.surface_view);

        result.encoder = begin(cmd_encoder, result.surface_view, depth);
        assert(result.encoder);

        return result;
    }

    static void end(RenderPass& pass)
    {
        wgpuRenderPassEncoderEnd(pass.encoder);
        wgpuRenderPassEncoderRelease(pass.encoder);
        wgpuTextureViewRelease(pass.surface_view);
        pass = {};
    }

  private:
    static WGPUTextureView make_view(WGPUSurface const surface)
    {
        WGPUSurfaceTexture srf_tex;
        wgpuSurfaceGetCurrentTexture(surface, &srf_tex);
        assert(srf_tex.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal);

        WGPUTextureViewDescriptor const desc{
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
        };
        return wgpuTextureCreateView(srf_tex.texture, &desc);
    }

    static WGPURenderPassEncoder begin(
        WGPUCommandEncoder const encoder,
        WGPUTextureView const surface_view,
        WGPUTextureView const depth_view)
    {
        WGPURenderPassColorAttachment color_atts[]{
            {
                .view = surface_view,
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue{0.15, 0.15, 0.15, 1.0},
            },
        };
        WGPURenderPassDepthStencilAttachment depth_atts[]{
            {
                .view = depth_view,
                .depthLoadOp = WGPULoadOp_Clear,
                .depthStoreOp = WGPUStoreOp_Store,
                .depthClearValue = 1.0f,
            },
        };
        WGPURenderPassDescriptor const desc{
            .colorAttachmentCount = 1,
            .colorAttachments = color_atts,
            .depthStencilAttachment = depth_atts,
        };
        return wgpuCommandEncoderBeginRenderPass(encoder, &desc);
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
        result.texture = make_texture(device, width, height, format);
        result.view = make_view(result.texture);
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

  private:
    static WGPUTexture make_texture(
        WGPUDevice const device,
        uint32_t const width,
        uint32_t const height,
        WGPUTextureFormat const format)
    {
        WGPUTextureDescriptor const desc{
            .usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc,
            .dimension = WGPUTextureDimension_2D,
            .size = {width, height, 1},
            .format = format,
            .mipLevelCount = 1,
            .sampleCount = 1,
        };
        return wgpuDeviceCreateTexture(device, &desc);
    }

    static WGPUTextureView make_view(WGPUTexture const texture)
    {
        WGPUTextureViewDescriptor const desc{
            .format = wgpuTextureGetFormat(texture),
            .dimension = WGPUTextureViewDimension_2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
        };
        return wgpuTextureCreateView(texture, &desc);
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
        result.vertices = make_buffer(
            device,
            vertex_data.size(),
            WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst);
        assert(result.vertices);

        result.indices = make_buffer(
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

  private:
    static WGPUBuffer make_buffer(
        WGPUDevice const device,
        size_t const size,
        WGPUBufferUsage const usage)
    {
        WGPUBufferDescriptor const desc{
            .usage = usage,
            .size = size,
            .mappedAtCreation = true,
        };
        return wgpuDeviceCreateBuffer(device, &desc);
    }
};

struct RenderMaterial
{
    static inline WGPUBindGroupLayout bind_group_layout{};
    static inline WGPUPipelineLayout pipeline_layout{};
    static inline WGPURenderPipeline pipeline{};
    struct
    {
        WGPUTexture texture;
        WGPUTextureView view;
        WGPUSampler sampler;
    } static inline color_map;

    WGPUBuffer uniform_buffer;
    WGPUBindGroup bind_group;
    struct
    {
        f32 local_to_clip[16];
    } uniforms{};

    static void init(WGPUDevice const device, WGPUTextureFormat const surface_format)
    {
        bind_group_layout = make_bind_group_layout(device);
        pipeline_layout = make_pipeline_layout(device, bind_group_layout);

        // Init pipeline
        {
            ShaderAsset const& asset = load_shader_asset("assets/shaders/unlit_texture.wgsl");
            pipeline = make_pipeline(
                device,
                pipeline_layout,
                {asset.src.c_str(), WGPU_STRLEN},
                surface_format,
                DepthTarget::format);
            assert(pipeline);
        }

        // Init color map
        {
            ImageAsset const& asset = load_image_asset("assets/images/cube-faces.png");
            color_map.texture = make_color_texture(
                device,
                asset.data.get(),
                asset.width,
                asset.height,
                asset.stride,
                color_map.view,
                color_map.sampler);
            assert(color_map.texture);
            assert(color_map.view);
            assert(color_map.sampler);
        }
    }

    static void deinit()
    {
        wgpuTextureViewRelease(color_map.view);
        wgpuSamplerRelease(color_map.sampler);
        wgpuTextureRelease(color_map.texture);
        color_map = {};

        wgpuRenderPipelineRelease(pipeline);
        pipeline = {};

        wgpuPipelineLayoutRelease(pipeline_layout);
        pipeline_layout = {};

        wgpuBindGroupLayoutRelease(bind_group_layout);
        bind_group_layout = {};
    }

    static RenderMaterial make(WGPUDevice const device)
    {
        RenderMaterial result{};

        result.uniform_buffer = make_uniform_buffer(device, sizeof(uniforms));
        assert(result.uniform_buffer);

        result.update_bind_group(device);
        return result;
    }

    static void release(RenderMaterial& material)
    {
        wgpuBufferRelease(material.uniform_buffer);
        wgpuBindGroupRelease(material.bind_group);
        material = {};
    }

    void update_bind_group(WGPUDevice const device)
    {
        if (bind_group)
            wgpuBindGroupRelease(bind_group);

        bind_group = make_bind_group(
            device,
            bind_group_layout,
            color_map.view,
            color_map.sampler,
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

  private:
    static WGPUBindGroupLayout make_bind_group_layout(WGPUDevice const device)
    {
        WGPUBindGroupLayoutEntry entries[]{
            {
                // Texture
                .binding = 0,
                .visibility = WGPUShaderStage_Fragment,
                .texture{
                    .sampleType = WGPUTextureSampleType_Float,
                    .viewDimension = WGPUTextureViewDimension_2D,
                    .multisampled = false,
                },
            },
            {
                // Sampler
                .binding = 1,
                .visibility = WGPUShaderStage_Fragment,
                .sampler{
                    .type = WGPUSamplerBindingType_Filtering,
                },
            },
            {
                // Uniforms
                .binding = 2,
                .visibility = WGPUShaderStage_Vertex,
                .buffer{
                    .type = WGPUBufferBindingType_Uniform,
                    .hasDynamicOffset = false,
                    .minBindingSize = 0,
                },
            },
        };
        WGPUBindGroupLayoutDescriptor const desc{
            .entryCount = size(entries),
            .entries = entries,
        };
        return wgpuDeviceCreateBindGroupLayout(device, &desc);
    }

    static WGPUPipelineLayout make_pipeline_layout(
        WGPUDevice const device,
        WGPUBindGroupLayout const bind_group_layout)
    {
        WGPUPipelineLayoutDescriptor const desc{
            .bindGroupLayoutCount = 1,
            .bindGroupLayouts = &bind_group_layout,
        };
        return wgpuDeviceCreatePipelineLayout(device, &desc);
    }

    static WGPURenderPipeline make_pipeline(
        WGPUDevice const device,
        WGPUPipelineLayout const layout,
        WGPUStringView const shader_src,
        WGPUTextureFormat const surface_format,
        WGPUTextureFormat const depth_format)
    {
        WGPUShaderSourceWGSL const shader_desc_src{
            .chain = {.sType = WGPUSType_ShaderSourceWGSL},
            .code = shader_src,
        };
        WGPUShaderModuleDescriptor const shader_desc{
            .nextInChain = as<WGPUChainedStruct>(&shader_desc_src),
        };
        WGPUShaderModule const shader = wgpuDeviceCreateShaderModule(device, &shader_desc);
        auto const drop_shader = defer([=]() { wgpuShaderModuleRelease(shader); });

        WGPUVertexAttribute const vert_attrs[]{
            {
                .format = WGPUVertexFormat_Float32x3,
                .offset = 0,
                .shaderLocation = 0,
            },
            {
                .format = WGPUVertexFormat_Float32x2,
                .offset = sizeof(float[3]),
                .shaderLocation = 1,
            },
        };
        WGPUVertexBufferLayout const vert_buf_layout{
            .stepMode = WGPUVertexStepMode_Vertex,
            .arrayStride = sizeof(float[5]),
            .attributeCount = size(vert_attrs),
            .attributes = vert_attrs,
        };

        WGPUDepthStencilState const depth_state{
            .format = depth_format,
            .depthWriteEnabled = WGPUOptionalBool_True,
            .depthCompare = WGPUCompareFunction_LessEqual,
        };

        WGPUColorTargetState const color_targ{
            .format = surface_format,
            .writeMask = WGPUColorWriteMask_All,
        };
        WGPUFragmentState const frag_state{
            .module = shader,
            .entryPoint = {"fs_main", WGPU_STRLEN},
            .targetCount = 1,
            .targets = &color_targ,
        };

        WGPURenderPipelineDescriptor const pipe_desc{
            .layout = layout,
            .vertex{
                .module = shader,
                .entryPoint = {"vs_main", WGPU_STRLEN},
                .bufferCount = 1,
                .buffers = &vert_buf_layout,
            },
            .primitive{
                .topology = WGPUPrimitiveTopology_TriangleList,
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = WGPUCullMode_None,
            },
            .depthStencil = &depth_state,
            .multisample{
                .count = 1,
                .mask = ~0u,
                .alphaToCoverageEnabled = 0u,
            },
            .fragment = &frag_state,
        };

        return wgpuDeviceCreateRenderPipeline(device, &pipe_desc);
    }

    static WGPUTexture make_color_texture(
        WGPUDevice const device,
        void* const data,
        uint32_t const width,
        uint32_t const height,
        uint32_t const stride,
        WGPUTextureView& view,
        WGPUSampler& sampler)
    {
        WGPUTextureFormat const format = WGPUTextureFormat_RGBA8Unorm;
        WGPUExtent3D const size = {width, height, 1};
        uint32_t const row_size = width * stride;
        uint32_t const data_size = row_size * height;

        WGPUTextureDescriptor const texture_desc{
            .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
            .dimension = WGPUTextureDimension_2D,
            .size = size,
            .format = format,
            .mipLevelCount = 1,
            .sampleCount = 1,
        };
        WGPUTexture const texture = wgpuDeviceCreateTexture(device, &texture_desc);

        // Copy image data to mip level 0
        WGPUTexelCopyTextureInfo const copy_info{
            .texture = texture,
        };
        WGPUTexelCopyBufferLayout const copy_layout{
            .bytesPerRow = row_size,
            .rowsPerImage = height,
        };
        WGPUQueue const queue = wgpuDeviceGetQueue(device);
        wgpuQueueWriteTexture(queue, &copy_info, data, data_size, &copy_layout, &size);

        // Create view
        WGPUTextureViewDescriptor const view_desc{
            .format = format,
            .dimension = WGPUTextureViewDimension_2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
        };
        view = wgpuTextureCreateView(texture, &view_desc);

        // Create sampler
        WGPUSamplerDescriptor const sampler_desc{
            .magFilter = WGPUFilterMode_Nearest,
            .minFilter = WGPUFilterMode_Nearest,
            .maxAnisotropy = 1,
        };
        sampler = wgpuDeviceCreateSampler(device, &sampler_desc);

        return texture;
    }

    static WGPUBuffer make_uniform_buffer(WGPUDevice const device, size_t const size)
    {
        WGPUBufferDescriptor const buf_desc{
            .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
            .size = size,
        };
        return wgpuDeviceCreateBuffer(device, &buf_desc);
    }

    static WGPUBindGroup make_bind_group(
        WGPUDevice const device,
        WGPUBindGroupLayout const layout,
        WGPUTextureView const color_view,
        WGPUSampler const color_sampler,
        WGPUBuffer const uniforms)
    {
        WGPUBindGroupEntry const entries[]{
            {
                .binding = 0,
                .textureView = color_view,
            },
            {
                .binding = 1,
                .sampler = color_sampler,
            },
            {
                .binding = 2,
                .buffer = uniforms,
                .size = wgpuBufferGetSize(uniforms),
            },
        };

        WGPUBindGroupDescriptor const bg_desc{
            .layout = layout,
            .entryCount = size(entries),
            .entries = entries,
        };
        return wgpuDeviceCreateBindGroup(device, &bg_desc);
    }
};

struct AppState
{
    GLFWwindow* window;
    GpuContext gpu;
    DepthTarget depth;
    RenderMaterial material;
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

    // Create WebGPU context and report details
    state.gpu = GpuContext::make({state.window, "#textured-mesh"});
    state.gpu.report();

    // Create additional render targets
    int fb_size[2];
    glfwGetFramebufferSize(state.window, fb_size, fb_size + 1);
    state.depth = DepthTarget::make(state.gpu.device, fb_size[0], fb_size[1]);

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
    RenderMaterial::init(state.gpu.device, default_surface_format);
    state.material = RenderMaterial::make(state.gpu.device);

    // Create mesh
    state.geometry = RenderMesh::make_box(state.gpu.device);
}

void deinit_app()
{
    RenderMesh::release(state.geometry);
    RenderMaterial::release(state.material);
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
    constexpr auto loop_cb = [](void* /*userdata*/) {
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

    MainLoop{state.gpu.surface, state.window, loop_cb}.begin();

    return 0;
}
