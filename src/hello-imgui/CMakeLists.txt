add_executable(
    hello-imgui
    main.cpp
    wgpu_config.c
)

target_link_libraries(
    hello-imgui
    PRIVATE 
        wgpu-app
)

if(EMSCRIPTEN)
    copy_web_src(hello-imgui)
endif()
