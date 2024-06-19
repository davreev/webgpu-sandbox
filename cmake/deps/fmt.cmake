if(TARGET fmt::fmt)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    fmt
    URL https://github.com/fmtlib/fmt/releases/download/10.2.1/fmt-10.2.1.zip
)

FetchContent_MakeAvailable(fmt)
