set(app_name hello-imgui)

add_executable(
    ${app_name}
    main.cpp
    wgpu_config.c
)

target_link_libraries(
    ${app_name}
    PRIVATE 
        wgpu-app
)

if(EMSCRIPTEN)
    set(src_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    set(
        web_src_files
        "${src_dir}/index.html"
        # ...
    )
    copy_web_src()
endif()
