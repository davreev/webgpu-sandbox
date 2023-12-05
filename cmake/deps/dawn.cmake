if(TARGET webgpu_dawn)
    return()
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE) # Show download progress

FetchContent_Declare(
    dawn
    GIT_REPOSITORY https://github.com/google/dawn
    GIT_TAG 90f47c00634bebbe5cedaee68a1b7e03473e191e
    GIT_PROGRESS TRUE
    GIT_SUBMODULES "" # Disables cloning of submodules
    # GIT_SHALLOW ON
)

set(DAWN_FETCH_DEPENDENCIES ON)
set(DAWN_BUILD_SAMPLES OFF)
set(TINT_BUILD_DOCS OFF)
set(TINT_BUILD_TESTS OFF)
FetchContent_MakeAvailable(dawn)
