if(TARGET dr::webgpu)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    dr-webgpu
    GIT_REPOSITORY git@github.com:davreev/dr-webgpu.git
    GIT_TAG 7b45039a75d3f834360b18e110cbead338591c76 # 0.1.0
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(dr-webgpu)
