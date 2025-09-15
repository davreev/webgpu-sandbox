add_library(
    app-base STATIC
    file_utils.cpp
    gfx_utils.cpp
)

include(deps/dr)
target_link_libraries(
    app-base
    PUBLIC
        dr::dr
        wgpu-app
)

add_subdirectory(buffer-copy)
add_subdirectory(clear-screen)
add_subdirectory(hello-imgui)
add_subdirectory(hello-triangle)
add_subdirectory(indexed-mesh)
add_subdirectory(textured-mesh)