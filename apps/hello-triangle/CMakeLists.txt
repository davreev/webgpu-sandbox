add_executable(
    hello-triangle 
    main.cpp
)

target_link_libraries(
    hello-triangle 
    PRIVATE 
        webgpu-app
)
