set(app_name indexed-mesh)

add_executable(
    ${app_name}
    config.c
    main.cpp
)

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
