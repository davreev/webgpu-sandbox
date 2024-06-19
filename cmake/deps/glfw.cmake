if(TARGET glfw::glfw)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
)

FetchContent_MakeAvailable(glfw)
add_library(glfw::glfw ALIAS glfw)
