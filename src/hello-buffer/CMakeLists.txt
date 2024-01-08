add_executable(
    hello-buffer 
    main.cpp
    wgpu_config.c
)

target_link_libraries(
    hello-buffer
    PRIVATE 
        wgpu-app
)
