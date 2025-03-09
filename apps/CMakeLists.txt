set(shared_src_dir "${CMAKE_CURRENT_LIST_DIR}/shared")

include(app-utils)

add_subdirectory(buffer-copy)
add_subdirectory(clear-screen)
add_subdirectory(hello-imgui)
add_subdirectory(hello-triangle)
add_subdirectory(indexed-mesh)
add_subdirectory(textured-mesh)
