if(TARGET glfw::glfw)
    return()
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE) # Show download progress

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 7482de6071d21db77a7236155da44c172a7f6c9e # 3.3.8
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(glfw)
add_library(glfw::glfw ALIAS glfw)
