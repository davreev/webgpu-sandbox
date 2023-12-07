add_executable(
    hello-webgpu 
    main.cpp
)

include(deps/fmt)

target_link_libraries(
    hello-webgpu
    PRIVATE
        fmt::fmt
        webgpu-glfw
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
