set(app_name "hello-triangle")

add_executable(
    ${app_name}
    main.cpp
)

target_link_libraries(
    ${app_name}
    PRIVATE
        app-base
)

if(EMSCRIPTEN)
    # Extra Emscripten compiler options
    target_link_options(
        ${app_name}
        PRIVATE
            "-sFORCE_FILESYSTEM=1"
            # ...
    )
endif()

#
# Post-build commands
#

include(app-utils)

set(asset_files)

if(EXAMPLES_USE_SLANG)
    set(
        slang_files
        "${src_dir}/assets/shaders/triangle.slang"
        # ...
    )
    set(wgsl_files)
    convert_shaders_slang_to_wgsl(wgsl_files)

    list(APPEND asset_files "${wgsl_files}")
else()
    set(
        wgsl_files
        "${src_dir}/assets/shaders/triangle.wgsl"
        # ...
    )
    list(APPEND asset_files "${wgsl_files}")
endif()

if(EMSCRIPTEN)
    set(
        web_src_files
        "${src_dir}/web/index.html"
        # ...
    )
    copy_web_files()
    package_assets()
else()
    copy_assets()
endif()
