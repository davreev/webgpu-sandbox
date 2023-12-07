if(TARGET dr::webgpu)
    return()
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE) # Show download progress

FetchContent_Declare(
    dr-webgpu
    GIT_REPOSITORY git@github.com:davreev/dr-webgpu.git
    GIT_TAG c99a6bdac7004990c7b10096dfc3b14f6ecac23e # 0.1.0
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(dr-webgpu)
