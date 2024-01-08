add_library(
    glfw-app STATIC
    "wgpu_glfw.c"
)

target_compile_options(
    glfw-app
    PRIVATE
        -Wall -Wextra -Wpedantic -Werror
)

include(deps/glfw)
target_link_libraries(
    glfw-app
    PUBLIC
        glfw::glfw
)

# Link to wgpu-native if not compiling with Emscripten
if(NOT EMSCRIPTEN)
    include(deps/wgpu-native)
    target_link_libraries(glfw-app PUBLIC wgpu-native)
endif()

if(APPLE)
    target_compile_options(glfw-app PRIVATE -x objective-c)
    target_link_libraries(
        glfw-app
        PRIVATE 
            "-framework Cocoa" 
            "-framework CoreVideo" 
            "-framework IOKit" 
            "-framework QuartzCore"
            "-framework Metal"
    )
endif()

add_library(
    app-base STATIC
    "wgpu_utils.cpp"
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

target_link_libraries(
    app-base
    PUBLIC
        fmt::fmt
        glfw-app
)
