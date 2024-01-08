add_executable(
    hello-mesh 
    main.cpp
    wgpu_config.c
)

target_link_libraries(
    hello-mesh 
    PRIVATE
        wgpu-app
)
