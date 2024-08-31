set(app_name textured-mesh)

add_executable(
    ${app_name}
    "assets.cpp"
    "graphics.c"
    "impl.cpp"
    "main.cpp"
    "${shared_src_dir}/file_utils.cpp"
    "${shared_src_dir}/gfx_utils.cpp"
)

include(deps/dr)
include(deps/stb-image)
target_link_libraries(
    ${app_name}
    PRIVATE 
        dr::dr
        stb::image
        wgpu-app
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

set(src_dir "${CMAKE_CURRENT_LIST_DIR}")
get_default_runtime_output_dir(runtime_output_dir)

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
