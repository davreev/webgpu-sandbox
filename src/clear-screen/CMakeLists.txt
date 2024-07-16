add_executable(
    clear-screen
    main.cpp
    wgpu_config.c
)

target_link_libraries(
    clear-screen
    PRIVATE 
        wgpu-app
)
