if(TARGET imgui::imgui)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.2.zip
)

FetchContent_GetProperties(imgui)
if(NOT ${imgui_POPULATED})
    FetchContent_Populate(imgui)
endif()

add_library(
    imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
)
add_library(imgui::imgui ALIAS imgui)

target_include_directories(
    imgui 
    PUBLIC
        "${imgui_SOURCE_DIR}"
)

add_library(
    imgui-wgpu STATIC
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp"
)

target_include_directories(
    imgui-wgpu
    PUBLIC
        "${imgui_SOURCE_DIR}/backends"
)

target_link_libraries(
    imgui-wgpu
    PUBLIC
        imgui::imgui
)

if(EMSCRIPTEN)
    # NOTE(dr): Using Emdawnwebgpu for Emscripten builds as its webgpu.h is more up to date
    include(deps/emdawnwebgpu)
    target_link_libraries(
        imgui
        PUBLIC
            emdawnwebgpu
    )
    # NOTE(dr): Dawn backend must be enabled to work with Emdawnwebgpu. This will cause an error
    # during Emscripten builds as ImGui expects neither native backend to be enabled. Disabling
    # the #error seems to be enough to work around the issue for now.
    #
    # Related PR: https://github.com/ocornut/imgui/pull/8831
    #
    target_compile_definitions(
        imgui
        PUBLIC
            IMGUI_IMPL_WEBGPU_BACKEND_DAWN
    )
else()
    # Link to native backend if not compiling with Emscripten
    include(deps/glfw)
    include(deps/wgpu-native)
    target_link_libraries(
        imgui-wgpu
        PRIVATE
            glfw::glfw
            wgpu-native
    )
    target_compile_definitions(
        imgui
        PUBLIC
            IMGUI_IMPL_WEBGPU_BACKEND_WGPU
    )
endif()
