if(TARGET dr::dr)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    dr
    URL https://github.com/davreev/dr/archive/refs/tags/0.5.0.zip
)

FetchContent_MakeAvailable(dr)
