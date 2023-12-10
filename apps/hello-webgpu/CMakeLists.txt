set(app_name hello-webgpu)

add_executable(
    ${app_name} 
    main.cpp
)

include(deps/fmt)

target_link_libraries(
    ${app_name}
    PRIVATE
        fmt::fmt
        webgpu-glfw
)

target_compile_options(
    ${app_name}
    PRIVATE 
        -Wall -Wextra -Wpedantic -Werror
)
