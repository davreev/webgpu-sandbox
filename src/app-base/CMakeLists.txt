add_library(
    app-base STATIC
    "wgpu_utils.cpp"
    "wgpu_glfw.c"
)

target_include_directories(
    app-base
    PUBLIC 
        "${CMAKE_CURRENT_LIST_DIR}"
)

target_compile_options(
    app-base
    PUBLIC 
        -Wall -Wextra -Wpedantic -Werror
)

include(deps/fmt)
include(deps/glfw)

target_link_libraries(
    app-base
    PUBLIC
        fmt::fmt
        glfw::glfw
)

# Link to wgpu-native if not compiling with Emscripten
if(NOT EMSCRIPTEN)
    include(deps/wgpu-native)
    target_link_libraries(app-base PUBLIC wgpu-native)
endif()

if(APPLE)
    target_compile_options(app-base PRIVATE -x objective-c)
    target_link_libraries(
        app-base
        PRIVATE 
            "-framework Cocoa" 
            "-framework CoreVideo" 
            "-framework IOKit" 
            "-framework QuartzCore"
    )
endif()
