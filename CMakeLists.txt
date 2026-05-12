cmake_minimum_required(VERSION 3.14)
project(
    webgpu-sandbox
    VERSION 0.4.0
    LANGUAGES C CXX
)

#
# Global config
#

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(
    PREPEND CMAKE_MODULE_PATH 
    "${CMAKE_CURRENT_LIST_DIR}/cmake"
)

# Show FetchContent download progress
set(FETCHCONTENT_QUIET FALSE)

# Suppress developer warnings
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 OLD)
endif()

#
# Target config
#

add_subdirectory(src)

option(WEBGPU_SANDBOX_EXAMPLES "Generate example targets" ON)
if(WEBGPU_SANDBOX_EXAMPLES)
    add_subdirectory(examples)
endif()