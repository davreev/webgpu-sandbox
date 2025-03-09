cmake_minimum_required(VERSION 3.14)
project(
    webgpu-sandbox
    VERSION 0.3.0
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

add_subdirectory(libs)

option(WEBGPU_SANDBOX_APPS "Generate app targets" ON)
if(WEBGPU_SANDBOX_APPS)
    add_subdirectory(apps)
endif()
