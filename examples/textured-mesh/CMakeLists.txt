set(app_name textured-mesh)

add_executable(
    ${app_name}
    "assets.cpp"
    "config.c"
    "impl.cpp"
    "main.cpp"
)

include(deps/dr)
include(deps/stb-image)
target_link_libraries(
    ${app_name}
    PRIVATE
        app-base
        stb::image
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

set(
    asset_files
    "${src_dir}/assets/shaders/unlit_texture.wgsl"
    "${src_dir}/assets/images/cube-faces.png"
    # ...
)

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
