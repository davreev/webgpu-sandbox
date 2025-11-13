if(TARGET dr::app-util)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    dr-app
    GIT_REPOSITORY https://github.com/davreev/dr-app.git
    GIT_TAG f7f6b3195634056716089c07929dd946f5db5cc2
)

set(DR_APP_UTIL_ONLY ON)
FetchContent_MakeAvailable(dr-app)
