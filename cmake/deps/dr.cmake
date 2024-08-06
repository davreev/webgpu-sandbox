if(TARGET dr::dr)
    return()
endif()

include(FetchContent)

# NOTE(dr): Pinned to specific revision until 0.4.0
#[[
FetchContent_Declare(
    dr
    URL https://github.com/davreev/dr/archive/refs/tags/0.3.0.zip
)
]]
FetchContent_Declare(
    dr
    GIT_REPOSITORY https://github.com/davreev/dr.git
    GIT_TAG 8f301117c336123fd0b4dc0e2bf72c32607b901a
)

FetchContent_MakeAvailable(dr)
