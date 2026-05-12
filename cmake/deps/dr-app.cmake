if(TARGET dr::app-util)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    dr-app
    GIT_REPOSITORY https://github.com/davreev/dr-app.git
    GIT_TAG 58bbfb8a2726d1c34cd738e390f03770e7c1a623
)

set(DR_APP_UTIL_ONLY ON)
FetchContent_MakeAvailable(dr-app)
