add_executable(
    indexed-mesh 
    main.cpp
    wgpu_config.c
)

target_link_libraries(
    indexed-mesh 
    PRIVATE
        wgpu-app
)

if(EMSCRIPTEN)
    copy_web_src(indexed-mesh)
endif()
