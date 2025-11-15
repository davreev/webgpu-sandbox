add_library(
    app-base STATIC
    example_base.cpp
)

include(deps/dr-app)
target_link_libraries(
    app-base
    PUBLIC
        dr::app-util
        wgpu-app
)

target_compile_options(
    app-base
    PUBLIC
        -Wall 
        -Wextra 
        -Wpedantic
        -Werror
)

add_subdirectory(clear-screen)
add_subdirectory(hello-compute)
add_subdirectory(hello-imgui)
add_subdirectory(hello-triangle)
add_subdirectory(hello-triangle-slang)
add_subdirectory(indexed-mesh)
add_subdirectory(textured-mesh)