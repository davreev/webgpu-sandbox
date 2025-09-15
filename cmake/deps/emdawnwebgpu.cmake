if(TARGET emdawnwebgpu)
    return()
endif()

function(fetch_emdawnwebgpu)
    set(version "v20250907.221810")

    include(FetchContent)

    FetchContent_Declare(
        emdawnwebgpu
        URL https://github.com/google/dawn/releases/download/${version}/emdawnwebgpu_pkg-${version}.zip
    )
    FetchContent_MakeAvailable(emdawnwebgpu)

    add_library(emdawnwebgpu INTERFACE)

    target_include_directories(
        emdawnwebgpu 
        INTERFACE
            "${emdawnwebgpu_SOURCE_DIR}/webgpu/include"
            # "${emdawnwebgpu_SOURCE_DIR}/webgpu_cpp/include"
    )
    target_compile_options(
        emdawnwebgpu 
        INTERFACE
            "--use-port=${emdawnwebgpu_SOURCE_DIR}/emdawnwebgpu.port.py"
    )
    target_link_options(
        emdawnwebgpu 
        INTERFACE
            "--use-port=${emdawnwebgpu_SOURCE_DIR}/emdawnwebgpu.port.py"
            "--closure-args=--externs=${emdawnwebgpu_SOURCE_DIR}/webgpu/src/webgpu-externs"
    )
endfunction()

fetch_emdawnwebgpu()
