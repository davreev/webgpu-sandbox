add_executable(
    hello-triangle 
    gfx_config.c
    main.cpp
)

target_link_libraries(
    hello-triangle 
    PRIVATE 
        webgpu-app
)
