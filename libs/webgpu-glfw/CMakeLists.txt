add_library(
    webgpu-glfw STATIC
    "src/glfw.c"
)

target_include_directories(
    webgpu-glfw
    PUBLIC 
        "${CMAKE_CURRENT_LIST_DIR}/include"
)

include(deps/dr-webgpu)
include(deps/glfw)

target_link_libraries(
    webgpu-glfw
    PUBLIC
        dr::webgpu
        glfw::glfw
)

target_compile_options(
    webgpu-glfw
    PRIVATE 
        -Wall -Wextra -Wpedantic -Werror
)

if(APPLE)
    target_compile_options(webgpu-glfw PRIVATE -x objective-c)
    target_link_libraries(
        webgpu-glfw
        PRIVATE 
            "-framework Cocoa" 
            "-framework CoreVideo" 
            "-framework IOKit" 
            "-framework QuartzCore"
    )
endif()
