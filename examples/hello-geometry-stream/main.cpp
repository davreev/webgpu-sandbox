#include <cassert>
#include <cmath>

#include <webgpu/webgpu.h>

#include <dr/container_utils.hpp>
#include <dr/dynamic_array.hpp>
#include <dr/linalg_reshape.hpp>
#include <dr/math.hpp>
#include <dr/math_types.hpp>
#include <dr/memory.hpp>
#include <dr/span.hpp>

#include <dr/app/gfx_utils.hpp>

#include "../assets.hpp"
#include "../example_app.hpp"
#include "../geometry_stream.hpp"
#include "../gpu_resource.hpp"
#include "../passes.hpp"

namespace wgpu::sandbox
{
namespace
{

using App = ExampleApp;

// TODO(dr): Wrap truncated octahedron in a struct

// Truncated octahedron: 24 vertices = permutations of (0, ±1, ±2).
constexpr i8 to_verts[24][3]{
    {0, 1, 2}, {0, 1, -2}, {0, -1, 2}, {0, -1, -2}, //  0..3
    {0, 2, 1}, {0, 2, -1}, {0, -2, 1}, {0, -2, -1}, //  4..7
    {1, 0, 2}, {1, 0, -2}, {-1, 0, 2}, {-1, 0, -2}, //  8..11
    {1, 2, 0}, {1, -2, 0}, {-1, 2, 0}, {-1, -2, 0}, // 12..15
    {2, 0, 1}, {2, 0, -1}, {-2, 0, 1}, {-2, 0, -1}, // 16..19
    {2, 1, 0}, {2, -1, 0}, {-2, 1, 0}, {-2, -1, 0}, // 20..23
};

// 14 faces (6 squares + 8 hexagons), triangulated as fans.
constexpr u32 to_tris[44][3]{
    // Square faces
    {20, 16, 21},
    {20, 21, 17}, // +x
    {22, 18, 23},
    {22, 23, 19}, // -x
    {12, 4, 14},
    {12, 14, 5}, // +y
    {13, 6, 15},
    {13, 15, 7}, // -y
    {8, 0, 10},
    {8, 10, 2}, // +z
    {9, 1, 11},
    {9, 11, 3}, // -z
    // Hexagonal faces (one per (sx,sy,sz) octant)
    {20, 12, 4},
    {20, 4, 0},
    {20, 0, 8},
    {20, 8, 16}, // +++
    {20, 12, 5},
    {20, 5, 1},
    {20, 1, 9},
    {20, 9, 17}, // ++-
    {21, 13, 6},
    {21, 6, 2},
    {21, 2, 8},
    {21, 8, 16}, // +-+
    {21, 13, 7},
    {21, 7, 3},
    {21, 3, 9},
    {21, 9, 17}, // +--
    {22, 14, 4},
    {22, 4, 0},
    {22, 0, 10},
    {22, 10, 18}, // -++
    {22, 14, 5},
    {22, 5, 1},
    {22, 1, 11},
    {22, 11, 19}, // -+-
    {23, 15, 6},
    {23, 6, 2},
    {23, 2, 10},
    {23, 10, 18}, // --+
    {23, 15, 7},
    {23, 7, 3},
    {23, 3, 11},
    {23, 11, 19}, // ---
};

constexpr u32 vertex_count{24};
constexpr u32 index_count{44 * 3};

// Lattice spacing of the truncated-octahedron tessellation. The TO above has
// hexagonal face centers at (±1, ±1, ±1), so adjacent cell centers along a
// (1,1,1)-type axis are 2*sqrt(3) apart; along an axis they are 4 apart.
constexpr f32 lattice_step{4.0f};

struct InstanceData
{
    f32 center[3];
    f32 scale;
    // TODO(dr): Add offsets for vertex pulling and skip dynamic offsets of vertex buffers
};

struct ObjectData
{
    f32 world_to_clip[16]{};
};

enum struct BindSlot : u8
{
    Pass = 0,
    Material,
    Geometry,
    Object,
};

struct
{
    DepthTarget depth;
    GpuRenderPipeline pipeline;
    GeometryStream geometry;
    struct
    {
        GpuBindGroupLayout bgl;
        GpuBindGroup bg;
        GpuBuffer buf;
    } uniforms;
    DynamicArray<InstanceData> instances;
    f32 fov_y{deg_to_rad(45.0f)};
    f32 clip_near{0.1f};
    f32 clip_far{200.0f};
} state;

// TODO(dr): Expose BG layout of geometry stream

#if false

// Mirrors the bind group layout that GeometryStream creates internally for its
// vertex storage buffers (4 read-only storage entries with dynamic offsets).
// The bind group returned by GeometryStream::bindings() is group-equivalent to
// this layout, so it can be bound to a pipeline that declares it.
WGPUBindGroupLayout make_geom_bgl(WGPUDevice const device)
{
    constexpr u32 slot_count{4};
    WGPUBindGroupLayoutEntry entries[slot_count]{};
    for (u32 i = 0; i < slot_count; ++i)
    {
        entries[i] = {
            .binding = i,
            .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .buffer{
                .type = WGPUBufferBindingType_ReadOnlyStorage,
                .hasDynamicOffset = true,
                .minBindingSize = 0,
            },
        };
    }
    WGPUBindGroupLayoutDescriptor const desc{
        .entryCount = slot_count,
        .entries = entries,
    };
    return wgpuDeviceCreateBindGroupLayout(device, &desc);
}
#endif

GpuBindGroupLayout empty_bgl;
GpuBindGroup empty_bg;

void init_empty_bindings(WGPUDevice device)
{
    WGPUBindGroupLayoutDescriptor const bgl_desc{
        .entryCount = 0,
        .entries = nullptr,
    };
    empty_bgl = wgpuDeviceCreateBindGroupLayout(device, &bgl_desc);

    WGPUBindGroupDescriptor const bg_desc{
        .layout = empty_bgl,
        .entryCount = 0,
        .entries = nullptr,
    };
    empty_bg = wgpuDeviceCreateBindGroup(device, &bg_desc);
}

WGPURenderPipeline make_pipeline(
    WGPUDevice const device,
    WGPUPipelineLayout const layout,
    WGPUStringView const shader_src,
    WGPUTextureFormat const color_format,
    WGPUTextureFormat const depth_format)
{
    WGPUShaderSourceWGSL shader_desc_src{
        .chain = {.sType = WGPUSType_ShaderSourceWGSL},
        .code = shader_src,
    };
    WGPUShaderModuleDescriptor const shader_desc{
        .nextInChain = as<WGPUChainedStruct>(&shader_desc_src),
    };
    GpuShaderModule const shader = wgpuDeviceCreateShaderModule(device, &shader_desc);

    WGPUDepthStencilState const depth_stencil{
        .format = depth_format,
        .depthWriteEnabled = WGPUOptionalBool_True,
        .depthCompare = WGPUCompareFunction_LessEqual,
    };
    WGPUColorTargetState const color_targ{
        .format = color_format,
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
            .entryPoint{"vs_main", WGPU_STRLEN},
        },
        .primitive{
            .topology = WGPUPrimitiveTopology_TriangleList,
            .frontFace = WGPUFrontFace_CCW,
            // TODO(dr): Fix this - should use consistent winding
            // Faces above don't use consistent winding, so leave culling off.
            .cullMode = WGPUCullMode_None,
        },
        .depthStencil = &depth_stencil,
        .multisample{
            .count = 1,
            .mask = ~0u,
        },
        .fragment = &frag_state,
    };
    return wgpuDeviceCreateRenderPipeline(device, &pipe_desc);
}

void init_instances()
{
    auto& xs = state.instances;
    xs.clear();

    // 3x3x3 cubic centers
    for (i32 a = -1; a <= 1; ++a)
    {
        for (i32 b = -1; b <= 1; ++b)
        {
            for (i32 c = -1; c <= 1; ++c)
                xs.push_back({{a * lattice_step, b * lattice_step, c * lattice_step}, 1.0f});
        }
    }

    // 2x2x2 body centers (shifted by half a lattice step)
    constexpr f32 h{lattice_step * 0.5f};
    for (i32 a = -1; a <= 0; ++a)
    {
        for (i32 b = -1; b <= 0; ++b)
        {
            for (i32 c = -1; c <= 0; ++c)
                xs.push_back(
                    {{a * lattice_step + h, b * lattice_step + h, c * lattice_step + h}, 1.0f});
        }
    }
}

void init_gpu_resources()
{
    WGPUDevice const device = App::gpu().device;

    int fb_w, fb_h;
    glfwGetFramebufferSize(App::window(), &fb_w, &fb_h);
    state.depth = DepthTarget::make(device, fb_w, fb_h);

    init_empty_bindings(device);
    GeometryStream::init_shared_resources(device);

    // Uniform buffer/bindings
    {
        WGPUBindGroupLayoutEntry const bgl_entries[]{
            {
                .binding = 0,
                .visibility = WGPUShaderStage_Vertex,
                .buffer{
                    .type = WGPUBufferBindingType_Uniform,
                    .hasDynamicOffset = false,
                    .minBindingSize = 0,
                },
            },
        };
        WGPUBindGroupLayoutDescriptor const bgl_desc{
            .entryCount = size(bgl_entries),
            .entries = bgl_entries,
        };
        WGPUBindGroupLayout const uniform_bgl = wgpuDeviceCreateBindGroupLayout(device, &bgl_desc);
        assert(uniform_bgl);

        WGPUBufferDescriptor const buf_desc{
            .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
            .size = sizeof(ObjectData),
        };
        WGPUBuffer const uniform_buf = wgpuDeviceCreateBuffer(device, &buf_desc);
        assert(uniform_buf);

        WGPUBindGroupEntry const bg_entries[]{
            {
                .binding = 0,
                .buffer = uniform_buf,
                .size = WGPU_WHOLE_SIZE,
            },
        };
        WGPUBindGroupDescriptor const bg_desc{
            .layout = uniform_bgl,
            .entryCount = size(bg_entries),
            .entries = bg_entries,
        };
        WGPUBindGroup const uniform_bg = wgpuDeviceCreateBindGroup(device, &bg_desc);
        assert(uniform_bg);

        state.uniforms = {
            .bgl = uniform_bgl,
            .bg = uniform_bg,
            .buf = uniform_buf,
        };
    }

    // Pipeline
    {
        WGPUBindGroupLayout const bg_layout[]{
            empty_bgl, // Pass
            empty_bgl, // Material
            GeometryStream::vertex_bindings_layout(), // Geometry
            state.uniforms.bgl, // Object
        };
        WGPUPipelineLayoutDescriptor const pl_desc{
            .bindGroupLayoutCount = size(bg_layout),
            .bindGroupLayouts = bg_layout,
        };
        GpuPipelineLayout const pl_layout = wgpuDeviceCreatePipelineLayout(device, &pl_desc);
        assert(pl_layout);

        ShaderAsset const* shader_src = load_shader_asset("assets/shaders/unlit_vertex_color.wgsl");
        assert(shader_src);

        state.pipeline = make_pipeline(
            device,
            pl_layout,
            {shader_src->src.c_str(), WGPU_STRLEN},
            default_surface_format,
            DepthTarget::format);
        assert(state.pipeline);
    }
}

void init()
{
    init_gpu_resources();
    init_instances();
}

f32 window_aspect()
{
    int w, h;
    glfwGetWindowSize(App::window(), &w, &h);
    return f32(w) / h;
}

Mat4<f32> make_world_to_clip()
{
    constexpr f64 cycles_per_frame{0.0008};
    f32 const theta = App::frame_count() * (cycles_per_frame * 2.0 * pi<f64>);
    constexpr f32 rad{18.0f};
    Vec3<f32> const eye{rad * std::cos(theta), 0.6f * rad, rad * std::sin(theta)};
    Mat4<f32> const world_to_view = make_look_at(eye, vec<3>(0.0f), vec(0.0f, 1.0f, 0.0f));
    Mat4<f32> const view_to_clip = make_perspective(
        state.fov_y,
        window_aspect(),
        state.clip_near,
        state.clip_far);
    return view_to_clip * world_to_view;
}

f32 instance_scale()
{
    // Oscillate between ~0.4 (gaps visible) and 1.0 (cells touching).
    constexpr f32 cycles_per_frame{0.005f};
    f32 const t = std::sin(App::frame_count() * (cycles_per_frame * 2.0f * pi<f32>));
    return 0.7f + 0.3f * t;
}

void update()
{
    WGPUDevice const device = App::gpu().device;
    WGPUQueue const queue = wgpuDeviceGetQueue(device);

    // Animate per-instance scale
    f32 const scale = instance_scale();
    for (auto& inst : state.instances)
        inst.scale = scale;

    // Pack per-vertex positions as float4 for std430 alignment
    f32 vertex_data[vertex_count][4]{};
    for (u32 i = 0; i < vertex_count; ++i)
    {
        vertex_data[i][0] = to_verts[i][0];
        vertex_data[i][1] = to_verts[i][1];
        vertex_data[i][2] = to_verts[i][2];
    }

    // Push geometry to the stream. The returned byte offsets are used as the matching dynamic
    // offsets at draw time; the caller decides which slot in geom.bindings() each one is bound to.
    auto& geom = state.geometry;
    u32 const vp_offset = geom.push_vertices(as<u8>(as_span(vertex_data)));
    u32 const ip_offset = geom.push_vertices(as<u8>(as_span(state.instances)));
    u32 const idx_offset = geom.push_indices(as<u8>(as_span(to_tris)));
    geom.update_device_buffers(device, queue);

    // Update pass uniforms
    Mat4<f32> const world_to_clip = make_world_to_clip();
    wgpuQueueWriteBuffer(queue, state.uniforms.buf, 0, world_to_clip.data(), sizeof(ObjectData));

    GpuCommandEncoder const cmd_encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    assert(cmd_encoder);

    {
        auto const pass = SurfaceRenderPass::make(
            cmd_encoder,
            App::gpu().surface,
            {0.08, 0.08, 0.1, 1.0},
            state.depth.view);

        wgpuRenderPassEncoderSetBindGroup(pass.encoder, u32(BindSlot::Pass), empty_bg, 0, nullptr);

        wgpuRenderPassEncoderSetBindGroup(
            pass.encoder,
            u32(BindSlot::Material),
            empty_bg,
            0,
            nullptr);

        // Bind the geometry stream's storage buffer with per-slot dynamic
        // offsets. Slots 2-3 are unused but the layout requires four offsets.
        u32 const offsets[4]{vp_offset, ip_offset, 0, 0};
        wgpuRenderPassEncoderSetBindGroup(
            pass.encoder,
            u32(BindSlot::Geometry),
            geom.vertex_bindings(),
            size(offsets),
            offsets);

        wgpuRenderPassEncoderSetPipeline(pass.encoder, state.pipeline);
        wgpuRenderPassEncoderSetBindGroup(
            pass.encoder,
            u32(BindSlot::Object),
            state.uniforms.bg,
            0,
            nullptr);

        wgpuRenderPassEncoderSetIndexBuffer(
            pass.encoder,
            geom.index_buffer(),
            GeometryStream::index_format,
            0,
            WGPU_WHOLE_SIZE);

        wgpuRenderPassEncoderDrawIndexed(
            pass.encoder,
            index_count,
            state.instances.size(),
            idx_offset / sizeof(u32),
            0,
            0);
    }

    GpuCommandBuffer const cmds = wgpuCommandEncoderFinish(cmd_encoder, nullptr);
    assert(cmds);
    wgpuQueueSubmit(queue, 1, &cmds.handle());

    geom.clear();
}

void handle_event(App::Event const& e)
{
    if (e.type == App::Event::Type::FramebufferResize)
    {
        auto const [w, h] = e.framebuffer_resize;
        state.depth = DepthTarget::make(App::gpu().device, w, h);
    }
}

} // namespace
} // namespace wgpu::sandbox

int main(int /*argc*/, char** /*argv*/)
{
    using namespace wgpu::sandbox;

    App::run({
        .init_cb = init,
        .frame_cb = update,
        .event_cb = handle_event,
        .window{
            .title = "WebGPU Sandbox: Hello Geometry Stream",
            .width = 800,
            .height = 600,
        },
        .html_canvas_id = "#hello-geometry-stream",
    });

    return 0;
}
