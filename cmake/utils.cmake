# Copies the given files from the source tree to the build tree via custom command. Returns paths of
# the copied files.
function(copy_to_bin files_in files_out)
    set(_files_out)
    
    foreach(file_in ${files_in})
        string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} file_out ${file_in})
        add_custom_command(
            OUTPUT
                "${file_out}"
            DEPENDS 
                "${file_in}"
            COMMAND 
                "${CMAKE_COMMAND}" -E copy 
                "${file_in}"
                "${file_out}"
            COMMENT 
                "Copying ${file_in}"
        )
        list(APPEND _files_out ${file_out})
    endforeach()

    set(${files_out} ${_files_out} PARENT_SCOPE)
endfunction()

# Copies web source files for the given target to the current binary dir and makes the target
# dependent on the output
function(copy_web_src app_name)
    set(
        files_in
        "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
        # ...
    )
    copy_to_bin("${files_in}" files_out)

    add_custom_target(${app_name}-web DEPENDS ${files_out})
    add_dependencies(${app_name} ${app_name}-web)
endfunction()
