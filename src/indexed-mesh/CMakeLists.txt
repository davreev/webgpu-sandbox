set(app_name indexed-mesh)

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
    set(
        web_src_files
        "${src_dir}/index.html"
        # ...
    )
    copy_web_src()
endif()
