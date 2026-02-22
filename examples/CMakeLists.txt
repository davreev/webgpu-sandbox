add_library(
    example-app STATIC
    example_app.cpp
    gpu_context.cpp
)

include(deps/dr-app)
target_link_libraries(
    example-app
    PUBLIC
        dr::app-util
        wgpu-app
)

target_compile_options(
    example-app
    PUBLIC
        -Wall 
        -Wextra 
        -Wpedantic
        -Werror
        # Allow anonymous types in anonymous unions
        -Wno-nested-anon-types
)

set(EXAMPLES_USE_SLANG ON)

find_program(slangc slangc)
if(NOT slangc)
    message(WARNING "Could not find slangc, falling back to wgsl shaders if available")
    set(EXAMPLES_USE_SLANG OFF)
endif()

add_subdirectory(clear-screen)
add_subdirectory(hello-compute)
add_subdirectory(hello-imgui)
add_subdirectory(hello-triangle)
add_subdirectory(indexed-mesh)
add_subdirectory(textured-mesh)