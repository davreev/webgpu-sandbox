add_executable(
    hello-triangle 
    main.cpp
    wgpu_config.c
)

target_link_libraries(
    hello-triangle 
    PRIVATE 
        app-base
)
