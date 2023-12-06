if(TARGET dr::webgpu)
    return()
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE) # Show download progress

FetchContent_Declare(
    dr-webgpu
    GIT_REPOSITORY git@github.com:davreev/dr-webgpu.git
    GIT_TAG 67170bcd5c88a77861d841d1a85a83c040823370 # 0.1.0
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(dr-webgpu)
