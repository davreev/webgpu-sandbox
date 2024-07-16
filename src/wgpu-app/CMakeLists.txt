add_library(
    wgpu-glfw STATIC
    "wgpu_glfw.c"
)

target_compile_options(
    wgpu-glfw
    PRIVATE
        -Wall -Wextra -Wpedantic -Werror
)

# Link to native backend if not compiling with Emscripten
if(NOT EMSCRIPTEN)
    include("deps/glfw")
    include("deps/wgpu-native")
    target_link_libraries(
        wgpu-glfw 
        PUBLIC
            glfw::glfw
            wgpu-native
        )
endif()

if(APPLE)
    target_compile_options(wgpu-glfw PRIVATE -x objective-c)
    target_link_libraries(
        wgpu-glfw
        PRIVATE 
            "-framework Cocoa" 
            "-framework CoreVideo" 
            "-framework IOKit" 
            "-framework QuartzCore"
            "-framework Metal"
    )
endif()

# Mirror private compile definitions from glfw
if (GLFW_BUILD_COCOA)
    target_compile_definitions(wgpu-glfw PRIVATE _GLFW_COCOA)
endif()
if (GLFW_BUILD_WIN32)
    target_compile_definitions(wgpu-glfw PRIVATE _GLFW_WIN32)
endif()
if (GLFW_BUILD_X11)
    target_compile_definitions(wgpu-glfw PRIVATE _GLFW_X11)
endif()
if (GLFW_BUILD_WAYLAND)
    target_compile_definitions(wgpu-glfw PRIVATE _GLFW_WAYLAND)
endif()

add_library(
    wgpu-app STATIC
    "wgpu_utils.cpp"
)

target_include_directories(
    wgpu-app
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
)

target_compile_options(
    wgpu-app
    PUBLIC
        -Wall -Wextra -Wpedantic -Werror
)

include("deps/fmt")
include("deps/imgui")
target_link_libraries(
    wgpu-app
    PUBLIC
        fmt::fmt
        imgui-wgpu
        wgpu-glfw
)
