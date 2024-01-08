add_library(
    wgpu-glfw STATIC
    "wgpu_glfw.c"
)

target_compile_options(
    wgpu-glfw
    PRIVATE
        -Wall -Wextra -Wpedantic -Werror
)

include(deps/glfw)
target_link_libraries(wgpu-glfw PUBLIC glfw::glfw)

# Link to wgpu-native if not compiling with Emscripten
if(NOT EMSCRIPTEN)
    include(deps/wgpu-native)
    target_link_libraries(wgpu-glfw PUBLIC wgpu-native)
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

include(deps/fmt)
target_link_libraries(
    wgpu-app
    PUBLIC
        fmt::fmt
        wgpu-glfw
)