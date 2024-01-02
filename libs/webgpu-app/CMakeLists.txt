add_library(
    webgpu-app STATIC
    "src/app.cpp"
)

target_include_directories(
    webgpu-app 
    PUBLIC 
        "${CMAKE_CURRENT_LIST_DIR}/include"
)

include(deps/fmt)

target_link_libraries(
    webgpu-app
    PUBLIC
        fmt::fmt
        webgpu-glfw
)

target_compile_options(
    webgpu-app
    PUBLIC 
        -Wall -Wextra -Wpedantic -Werror
)
