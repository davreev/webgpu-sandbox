set(app_name hello-imgui)

add_executable(
    ${app_name}
    main.cpp
    graphics.c
)

include(deps/dr)
target_link_libraries(
    ${app_name}
    PRIVATE
        dr::dr
        wgpu-app
)

#
# Post-build commands
#

set(src_dir "${CMAKE_CURRENT_LIST_DIR}")
get_default_runtime_output_dir(runtime_output_dir)

if(EMSCRIPTEN)
    set(
        web_src_files
        "${src_dir}/web/index.html"
        # ...
    )
    copy_web_files()
endif()
