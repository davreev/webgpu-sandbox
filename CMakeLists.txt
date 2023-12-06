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

# add_subdirectory(apps/hello-triangle)
add_subdirectory(apps/hello-webgpu)
