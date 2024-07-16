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
