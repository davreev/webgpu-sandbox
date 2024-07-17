add_executable(
    hello-triangle 
    main.cpp
    wgpu_config.c
)

target_link_libraries(
    hello-triangle 
    PRIVATE 
        wgpu-app
)

if(EMSCRIPTEN)
    copy_web_src(hello-triangle)
endif()
