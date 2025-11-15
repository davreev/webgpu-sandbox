if(TARGET dr::app-util)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    dr-app
    GIT_REPOSITORY https://github.com/davreev/dr-app.git
    GIT_TAG 30c49949f37af48455ae711cbdb50b8188342c23
)

set(DR_APP_UTIL_ONLY ON)
FetchContent_MakeAvailable(dr-app)
