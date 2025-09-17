if(TARGET wgvk::wgvk)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    wgvk
    URL https://github.com/manuel5975p/WGVK/archive/refs/tags/v0.0.1.zip
)

# NOTE(dr): wgvk only supports spir-v by default
set(WGVK_BUILD_WGSL_SUPPORT ON)

FetchContent_MakeAvailable(wgvk)
add_library(wgvk::wgvk ALIAS wgvk)
