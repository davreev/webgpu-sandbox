if(TARGET wgpu-native)
    return()
endif()

set(_release_ver "v0.18.1.3")
set(_release_arch ${CMAKE_HOST_SYSTEM_PROCESSOR})

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
    set(_release_file "wgpu-linux-${_release_arch}-release.zip")
    set(_lib_file "libwgpu_native.a")

    if(${_release_arch} STREQUAL "aarch64")
        set(_release_hash "6801035226d0b747f88fc667e75fd793af8779c1fde6a2fae1bffc28c1283f95")
    elseif(${_release_arch} STREQUAL "x86_64")
        set(_release_hash "d683abcad4b834edc0fe5d09c5919758a21fc8ee11feaf7a0d05ecbfcaace0f8")
    else()
        message(WARNING "Precompiled binaries for wgpu-native are not available for this platform")
    endif()

elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Darwin")
    set(_release_file "wgpu-macos-${_release_arch}-release.zip")
    set(_lib_file "libwgpu_native.a")

    if(${_release_arch} STREQUAL "aarch64")
        set(_release_hash "2ec1583558836101c3ce21e2bc90361be3e52acd72f976cea09d64b177a9eb1b")
    elseif(${_release_arch} STREQUAL "x86_64")
        set(_release_hash "2126b1746ea2ba8503a7f56a58fda8dcc85c8b2cf126113d4789b68db9c7e5be")
    else()
        message(WARNING "Precompiled binaries for wgpu-native are not available for this platform")
    endif()

elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
    set(_release_file "wgpu-windows-${_release_arch}-release.zip")
    set(_lib_file "libwgpu_native.lib")

    if(${_release_arch} STREQUAL "i686")
        set(_release_hash "21d4b5ced28e9975c2eeab3757f217373747cc2036ec06af223ec0781d2a239d")
    elseif(${_release_arch} STREQUAL "x86_64")
            set(_release_hash "f2dc11534cd210c33cc517b40ca8f8ee30ce9d498ff3f2e0e5bc98000548cc5a")
    else()
        message(WARNING "Precompiled binaries for wgpu-native are not available for this platform")
    endif()

else()
    message(WARNING "Precompiled binaries for wgpu-native are not available for this platform")

endif()

include(FetchContent)

FetchContent_Declare(
    wgpu-native
    URL "https://github.com/gfx-rs/wgpu-native/releases/download/${_release_ver}/${_release_file}"
    URL_HASH "SHA256=${_release_hash}"
)

FetchContent_GetProperties(wgpu-native)
if(NOT wgpu-native_POPULATED)
    FetchContent_Populate(wgpu-native)

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
        IMPORTED_LOCATION "${wgpu-native_SOURCE_DIR}/${_lib_file}"
        INTERFACE_INCLUDE_DIRECTORIES "${wgpu-native_SOURCE_DIR}/include"
    )

endif()
