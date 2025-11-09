set(app_name "hello-triangle-slang")

find_program(slangc slangc)
if(NOT slangc)
    message(WARNING "Excluding ${app_name} from build as it requires slangc which cannot be found")
    return()
endif()

add_executable(
    ${app_name}
    config.c
    main.cpp
)

include(deps/dr)
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

set(
    slang_files
    "${src_dir}/assets/shaders/triangle.slang"
)

set(gen_asset_files)
convert_shaders_slang_to_wgsl(gen_asset_files)

set(
    asset_files
    ${gen_asset_files}
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
