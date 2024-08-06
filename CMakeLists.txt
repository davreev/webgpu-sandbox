cmake_minimum_required(VERSION 3.14)
project(
    webgpu-sandbox
    VERSION 0.2.0
    LANGUAGES C CXX
)

#
# Global config
#

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

list(
    PREPEND CMAKE_MODULE_PATH 
    "${CMAKE_CURRENT_LIST_DIR}/cmake"
)
include(app-utils)

# Show FetchContent download progress
set(FETCHCONTENT_QUIET FALSE)

#
# Targets
#

set(shared_src_dir "${CMAKE_CURRENT_LIST_DIR}/src/shared")

# Base library
add_subdirectory("src/wgpu-app")

# Examples
# add_subdirectory("src/buffer-copy")
# add_subdirectory("src/clear-screen")
# add_subdirectory("src/hello-imgui")
# add_subdirectory("src/hello-triangle")
# add_subdirectory("src/indexed-mesh")
add_subdirectory("src/textured-mesh")
