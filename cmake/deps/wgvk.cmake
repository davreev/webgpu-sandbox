if(TARGET wgvk::wgvk)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    wgvk
    GIT_REPOSITORY https://github.com/manuel5975p/WGVK.git
    GIT_TAG 4d968237d3b37a87eda6f46749226e3930dc18f0
)

# NOTE(dr): wgvk only supports spir-v by default
set(WGVK_BUILD_WGSL_SUPPORT OFF)
set(WGVK_BUILD_GLSL_SUPPORT OFF)
set(BUILD_TESTING OFF)

FetchContent_MakeAvailable(wgvk)
add_library(wgvk::wgvk ALIAS wgvk)
