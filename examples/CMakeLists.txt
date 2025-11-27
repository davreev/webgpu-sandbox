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