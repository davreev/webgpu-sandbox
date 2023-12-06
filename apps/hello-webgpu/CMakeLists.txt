add_executable(
    hello-webgpu 
    main.cpp
)

include(deps/dr-webgpu)
include(deps/fmt)
include(deps/glfw)

target_link_libraries(
    hello-webgpu
    PRIVATE
        dr::webgpu
        fmt::fmt
        glfw::glfw
)

target_compile_options(
    hello-webgpu
    PRIVATE 
        -Wall -Wextra -Wpedantic -Werror
        # DEBUG
        -Wno-unused-function
)

target_compile_definitions(
    hello-webgpu
    PRIVATE 
        # ...
)

target_link_options(
    hello-webgpu
    PRIVATE
        # ...
)
