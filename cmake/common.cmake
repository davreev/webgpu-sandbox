function(get_default_runtime_output_dir result_)
    if(CMAKE_CONFIGURATION_TYPES)
        set(result "${CMAKE_CURRENT_BINARY_DIR}/\$<CONFIG>")
    else()
        set(result "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
    set(${result_} ${result} PARENT_SCOPE)
endfunction()


function(copy_web_src)
    get_default_runtime_output_dir(dst_dir)

    set(files_out)
    foreach(file_in ${web_src_files})
        file(RELATIVE_PATH rel_path "${src_dir}" ${file_in})
        set(file_out "${dst_dir}/${rel_path}")
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
