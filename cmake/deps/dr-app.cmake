if(TARGET dr::app-util)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    dr-app
    GIT_REPOSITORY https://github.com/davreev/dr-app.git
    GIT_TAG 3c1a88be3675af8f37e628719190536eb9362c2e
)

set(DR_APP_UTIL_ONLY ON)
FetchContent_MakeAvailable(dr-app)
