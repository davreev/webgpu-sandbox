set(app_name hello-imgui)

add_executable(
    ${app_name}
    config.c
    main.cpp
)

include(deps/dr)
target_link_libraries(
    ${app_name}
    PRIVATE
        app-base
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
