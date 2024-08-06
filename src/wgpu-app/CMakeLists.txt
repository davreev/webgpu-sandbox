add_library(
    wgpu-app STATIC
    "wgpu_utils.cpp"
)

target_include_directories(
    wgpu-app
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
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
    PUBLIC
        -Wall -Wextra -Wpedantic -Werror
)

# Set Emscripten compiler options
if(EMSCRIPTEN)
    target_link_options(
        wgpu-app
        PUBLIC
            "-sUSE_WEBGPU=1"
            "-sUSE_GLFW=3"
            "-sASYNCIFY"
            "$<$<CONFIG:Debug>:-sASSERTIONS=2>"
            "$<$<CONFIG:Debug>:-gsource-map>"
    )
endif()

# Link to native backend if not compiling with Emscripten
if(NOT EMSCRIPTEN)
    add_library(
        wgpu-glfw STATIC
        "wgpu_glfw.c"
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
