include(deps/fmt)

add_executable(
    learn-webgpu-cpp 
    main.cpp
)

target_link_libraries(
    learn-webgpu-cpp 
    PRIVATE 
        fmt::fmt 
        webgpu-glfw
)

target_compile_options(
    learn-webgpu-cpp 
        PRIVATE 
            -Wall -Wextra -Wpedantic -Werror
)
