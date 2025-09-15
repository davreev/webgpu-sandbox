if(TARGET imgui::imgui)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    imgui
    URL https://github.com/davreev/imgui/archive/refs/tags/v1.92.2b-emdawnwebgpu.zip
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
    # NOTE(dr): Using a fork of ImGui that enables use of Emdawnwebgpu
    target_compile_definitions(
        imgui
        PUBLIC
            IMGUI_IMPL_WEBGPU_EMDAWNWEBGPU

    )
else()
    # Link to native backend if not compiling with Emscripten
    include(deps/glfw)
    include(deps/wgpu-native)
    # include(deps/wgvk)
    target_link_libraries(
        imgui-wgpu
        PRIVATE
            glfw::glfw
            wgpu-native
            # wgvk::wgvk
    )
    target_compile_definitions(
        imgui
        PUBLIC
            IMGUI_IMPL_WEBGPU_BACKEND_WGPU
    )
endif()
