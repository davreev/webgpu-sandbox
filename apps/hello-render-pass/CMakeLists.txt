add_executable(
    hello-render-pass 
    main.cpp
)

target_link_libraries(
    hello-render-pass 
    PRIVATE 
        webgpu-app
)
