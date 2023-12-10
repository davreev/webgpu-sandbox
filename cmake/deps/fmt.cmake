if(TARGET fmt::fmt)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG c4ee726532178e556d923372f29163bd206d7732 # 9.0.0
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(fmt)
