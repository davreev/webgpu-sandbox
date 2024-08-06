function(get_default_runtime_output_dir result_)
    if(CMAKE_CONFIGURATION_TYPES)
        set(result "${CMAKE_CURRENT_BINARY_DIR}/\$<CONFIG>")
    else()
        set(result "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
    set(${result_} ${result} PARENT_SCOPE)
endfunction()


function(get_base_dir file result_)
    if(file MATCHES "^${gen_dir}")
        set(result ${gen_dir})
    elseif(file MATCHES "^${src_dir}")
        set(result ${src_dir})
    elseif(file MATCHES "^${shared_src_dir}")
        set(result ${shared_src_dir})
    else()
        message(FATAL_ERROR "Unexpected file location: ${file}")
    endif()
    set(${result_} ${result} PARENT_SCOPE)
endfunction()


function(copy_assets)
    set(files_out)
    foreach(file_in ${asset_files})
        get_base_dir(${file_in} base_dir)
        file(RELATIVE_PATH rel_path ${base_dir} ${file_in})
        set(file_out "${runtime_output_dir}/${rel_path}")
        add_custom_command(
            OUTPUT
                ${file_out}
            DEPENDS 
                ${file_in}
            COMMAND 
                ${CMAKE_COMMAND} -E copy 
                ${file_in}
                ${file_out}
            COMMENT 
                "Copying ${file_in}"
        )
        list(APPEND files_out ${file_out})
    endforeach()
    
    add_custom_target(${app_name}-assets DEPENDS ${files_out})
    add_dependencies(${app_name} ${app_name}-assets)
endfunction()


function(package_assets)
    if(NOT asset_files)
        return()
    endif()
    
    # Create Emscripten file mappings
    set(file_mappings)
    foreach(file ${asset_files})
        get_base_dir(${file} base_dir)
        file(RELATIVE_PATH rel_path ${base_dir} ${file})
        list(APPEND file_mappings "${file}@/${rel_path}")
    endforeach()

    # Create asset package via Emscripten's file packager utility
    set(emsc_file_packager "${EMSCRIPTEN_ROOT_PATH}/tools/file_packager")
    set(output "${runtime_output_dir}/${app-name}-assets.data")
    add_custom_command(
        OUTPUT
            ${output}
        DEPENDS
            ${asset_files}
        COMMAND
            ${emsc_file_packager}
            ${app_name}-assets.data 
            --js-output=${app_name}-assets.js 
            --preload
            ${file_mappings}
        WORKING_DIRECTORY
            "${runtime_output_dir}"
        COMMENT
            "Packaging asset data"
    )

    add_custom_target(${app_name}-assets-web DEPENDS ${output})
    add_dependencies(${app_name} ${app_name}-assets-web)
endfunction()


function(copy_web_files)
    set(files_out)
    foreach(file_in ${web_src_files})
        file(RELATIVE_PATH rel_path "${src_dir}/web" ${file_in})
        set(file_out "${runtime_output_dir}/${rel_path}")
        add_custom_command(
            OUTPUT
                ${file_out}
            DEPENDS 
                ${file_in}
            COMMAND 
                ${CMAKE_COMMAND} -E copy 
                ${file_in}
                ${file_out}
            COMMENT 
                "Copying ${file_in}"
        )
        list(APPEND files_out ${file_out})
    endforeach()

    add_custom_target(${app_name}-web DEPENDS ${files_out})
    add_dependencies(${app_name} ${app_name}-web)
endfunction()
