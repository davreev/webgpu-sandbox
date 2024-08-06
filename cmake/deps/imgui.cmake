if(TARGET imgui::imgui)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.90.9.zip
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

# Link to native backend if not compiling with Emscripten
if(NOT EMSCRIPTEN)
    include(deps/glfw)
    include(deps/wgpu-native)
    target_link_libraries(
        imgui-wgpu
        PRIVATE
            glfw::glfw
            wgpu-native
    )
endif()
