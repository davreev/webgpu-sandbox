if(TARGET dr::dr)
    return()
endif()

include(FetchContent)

# DEBUG(dr): Use later version that what's brought in by dr-app
FetchContent_Declare(
    dr
    GIT_REPOSITORY https://github.com/davreev/dr.git
    GIT_TAG 081279a393b492a9dbadc6b8374445a447811420
)

FetchContent_MakeAvailable(dr)
