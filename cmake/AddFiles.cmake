include_guard(GLOBAL)

function(add_files target_name)
    if(NOT TARGET "${target_name}")
        message(FATAL_ERROR "add_files target '${target_name}' does not exist")
    endif()

    set(files)
    foreach(file IN LISTS ARGN)
        if(IS_ABSOLUTE "${file}")
            list(APPEND files "${file}")
        else()
            list(APPEND files "${CMAKE_CURRENT_LIST_DIR}/${file}")
        endif()
    endforeach()

    target_sources("${target_name}" PRIVATE ${files})

    if(files)
        source_group(TREE "${PROJECT_SOURCE_DIR}" FILES ${files})
    endif()
endfunction()
