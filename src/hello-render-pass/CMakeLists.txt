add_executable(
    hello-render-pass 
    main.cpp
    wgpu_config.c
)

target_link_libraries(
    hello-render-pass 
    PRIVATE 
        app-base
)
