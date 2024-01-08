cmake_minimum_required(VERSION 3.14)
project(
    webgpu-sandbox
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

list(
    PREPEND CMAKE_MODULE_PATH 
    "${CMAKE_CURRENT_LIST_DIR}/cmake"
)

# Show FetchContent download progress
set(FETCHCONTENT_QUIET FALSE)

add_subdirectory(src/wgpu-app)
add_subdirectory(src/hello-webgpu)
add_subdirectory(src/hello-buffer)
add_subdirectory(src/hello-render-pass)
add_subdirectory(src/hello-triangle)
add_subdirectory(src/hello-mesh)
