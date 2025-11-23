add_library(
    wgpu-app STATIC
    wgpu_utils.cpp
)

if(EMSCRIPTEN)
    target_sources(wgpu-app PRIVATE emsc_utils.cpp)
endif()

target_include_directories(
    wgpu-app
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

include(deps/fmt)
include(deps/imgui)
target_link_libraries(
    wgpu-app
    PUBLIC
        fmt::fmt
        imgui-wgpu
)

target_compile_options(
    wgpu-app
    PRIVATE
        -Wall -Wextra -Wpedantic -Werror
)

if(EMSCRIPTEN)
    # NOTE(dr): Using Emdawnwebgpu for Emscripten builds as its webgpu.h is more up to date
    include(deps/emdawnwebgpu)
    target_link_libraries(
        wgpu-app
        PUBLIC
            emdawnwebgpu
    )

    # Set Emscripten compiler options
    target_link_options(
        wgpu-app
        PUBLIC
            # "-sUSE_WEBGPU=1"
            "-sUSE_GLFW=3"
            "-sASYNCIFY"
            "$<$<CONFIG:Debug>:-sASSERTIONS=2>"
            "$<$<CONFIG:Debug>:-gsource-map>"
    )

    # NOTE(dr): Emscripten Clang incorrectly warns on these
    # (https://github.com/llvm/llvm-project/issues/68933)
    target_compile_options(
        wgpu-app
        PUBLIC
            -Wno-missing-designated-field-initializers
    )
else()
    # Link to native backend if not compiling with Emscripten
    add_library(
        wgpu-glfw STATIC
        wgpu_glfw.c
    )

    include(deps/glfw)
    include(deps/wgpu-native)
    target_link_libraries(
        wgpu-glfw 
        PUBLIC
            glfw::glfw
            wgpu-native
    )

    if(APPLE)
        target_compile_options(wgpu-glfw PRIVATE -x objective-c)
        target_link_libraries(
            wgpu-glfw
            PRIVATE 
                "-framework Cocoa" 
                "-framework CoreVideo" 
                "-framework IOKit" 
                "-framework QuartzCore"
                "-framework Metal"
        )
    endif()

    target_compile_options(
        wgpu-glfw
        PRIVATE
            -Wall -Wextra -Wpedantic -Werror
    )

    # Mirror private compile definitions from glfw
    if (GLFW_BUILD_COCOA)
        target_compile_definitions(wgpu-glfw PRIVATE _GLFW_COCOA)
    endif()
    if (GLFW_BUILD_WIN32)
        target_compile_definitions(wgpu-glfw PRIVATE _GLFW_WIN32)
    endif()
    if (GLFW_BUILD_X11)
        target_compile_definitions(wgpu-glfw PRIVATE _GLFW_X11)
    endif()
    if (GLFW_BUILD_WAYLAND)
        target_compile_definitions(wgpu-glfw PRIVATE _GLFW_WAYLAND)
    endif()

    target_link_libraries(
        wgpu-app
        PUBLIC
            wgpu-glfw
    )
endif()
