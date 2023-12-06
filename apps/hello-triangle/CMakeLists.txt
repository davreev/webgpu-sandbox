add_executable(
    hello-triangle 
    main.cpp
)

include(deps/fmt)
include(deps/glfw)

target_link_libraries(
    hello-triangle
    PRIVATE
        fmt::fmt
        glfw::glfw
)

target_compile_options(
    hello-triangle
    PRIVATE 
        -Wall -Wextra -Wpedantic -Werror
)

target_compile_definitions(
    hello-triangle
    PRIVATE 
        # ...
)

target_link_options(
    hello-triangle
    PRIVATE
        # ...
)

if(EMSCRIPTEN)
    # app_default_emcc_options(emcc_options)

    # Emscripten compiler options are set via CMake link flags (https://stackoverflow.com/a/37869647/5113120)
    set_target_properties(
        hello-triangle
        PROPERTIES
            LINK_FLAGS ${emcc_options}
    )
endif()

