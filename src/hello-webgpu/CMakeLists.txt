add_executable(
    hello-webgpu 
    main.cpp
)

target_link_libraries(
    hello-webgpu
    PRIVATE
        app-base
)