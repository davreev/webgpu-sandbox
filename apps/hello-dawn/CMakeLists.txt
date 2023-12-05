add_executable(
    hello-dawn 
    main.cpp
)

include(deps/fmt)
include(deps/glfw) # Included before dawn since we want to use a custom glfw

include(deps/dawn)

target_link_libraries(
    hello-dawn
    PRIVATE
        fmt::fmt
        webgpu_cpp
        webgpu_dawn
        webgpu_glfw
)

target_compile_options(
    hello-dawn
    PRIVATE 
        -Wall -Wextra -Wpedantic -Werror
        # DEBUG
        -Wno-unused-function
)

target_compile_definitions(
    hello-dawn
    PRIVATE 
        # ...
)

target_link_options(
    hello-dawn
    PRIVATE
        # ...
)

