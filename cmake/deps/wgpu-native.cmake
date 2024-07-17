if(TARGET wgpu-native)
    return()
endif()

function(import_wgpu_native)
    set(release_ver "v0.19.4.1")

    set(release_arch ${CMAKE_HOST_SYSTEM_PROCESSOR})
    if(${release_arch} STREQUAL "arm64")
        set(release_arch "aarch64")
    endif()

    set(error_msg "Precompiled binaries for wgpu-native are not available for this platform")

    if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
        set(release_file "wgpu-linux-${release_arch}-release.zip")
        set(lib_file "libwgpu_native.a")

        if(NOT(${release_arch} STREQUAL "aarch64" OR ${release_arch} STREQUAL "x86_64"))
            message(FATAL_ERROR "${error_msg}")
        endif()

    elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Darwin")
        set(release_file "wgpu-macos-${release_arch}-release.zip")
        set(lib_file "libwgpu_native.a")

        if(NOT(${release_arch} STREQUAL "aarch64" OR ${release_arch} STREQUAL "x86_64"))
            message(FATAL_ERROR "${error_msg}")
        endif()

    elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
        set(release_file "wgpu-windows-${release_arch}-release.zip")
        set(lib_file "libwgpu_native.lib")

        if(NOT(${release_arch} STREQUAL "i686" OR ${release_arch} STREQUAL "x86_64"))
            message(FATAL_ERROR "${error_msg}")
        endif()

    else()
        message(FATAL_ERROR "${error_msg}")

    endif()

    include(FetchContent)

    FetchContent_Declare(
        wgpu-native
        URL "https://github.com/gfx-rs/wgpu-native/releases/download/${release_ver}/${release_file}"
    )

    FetchContent_GetProperties(wgpu-native)
    if(NOT wgpu-native_POPULATED)
        FetchContent_Populate(wgpu-native)
    endif()

    add_library(wgpu-native STATIC IMPORTED)

    # WebGPU headers are expected to be located at webgpu/*.h
    file(
        COPY 
            "${wgpu-native_SOURCE_DIR}/webgpu.h" 
            "${wgpu-native_SOURCE_DIR}/wgpu.h"
        DESTINATION 
            "${wgpu-native_SOURCE_DIR}/include/webgpu"
    )

    set_target_properties(wgpu-native PROPERTIES
        IMPORTED_LOCATION "${wgpu-native_SOURCE_DIR}/${lib_file}"
        INTERFACE_INCLUDE_DIRECTORIES "${wgpu-native_SOURCE_DIR}/include"
    )

endfunction()

import_wgpu_native()
