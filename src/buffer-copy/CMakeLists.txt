add_executable(
    buffer-copy 
    main.cpp
    wgpu_config.c
)

target_link_libraries(
    buffer-copy
    PRIVATE 
        wgpu-app
)

if(EMSCRIPTEN)
    copy_web_src(buffer-copy)
endif()
