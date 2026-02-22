set(app_name clear-screen)

add_executable(
    ${app_name}
    main.cpp
)

target_link_libraries(
    ${app_name}
    PRIVATE
        example-app
)

#
# Post-build commands
#

include(app-utils)

if(EMSCRIPTEN)
    set(
        web_src_files
        "${src_dir}/web/index.html"
        # ...
    )
    copy_web_files()
endif()
