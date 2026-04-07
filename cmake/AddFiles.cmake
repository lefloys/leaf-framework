# Add a file to be compiled.
#
# add_files([target] [file1 ...] CONDITION condition [condition ...])
#
# TARGET is the name of the target to which the source files will be added.
# CONDITION is a complete statement that can be evaluated with if().
# If it evaluates true, the source files will be added; otherwise not.
# For example: ADD_IF SDL_FOUND AND Allegro_FOUND
#
function(add_files tgt)
    cmake_parse_arguments(PARAM "" "" "CONDITION" ${ARGN})
    set(PARAM_FILES "${PARAM_UNPARSED_ARGUMENTS}")

    if(PARAM_CONDITION)
        if(NOT ${PARAM_CONDITION})
            return()
        endif()
    endif()

    get_target_property(current_files ${tgt} SOURCES)
    if(NOT current_files)
        set(current_files "")
    endif()

    foreach(FILE IN LISTS PARAM_FILES)
        set(full_path "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}")

        list(FIND current_files ${full_path} file_index)
        if(file_index GREATER -1)
            if(NOT (${FILE} MATCHES "\\.h$" AND full_path MATCHES "3rdparty"))
                message(FATAL_ERROR "${tgt}: ${full_path} is a duplicate of ${current_files}")
            endif()
        else()
            target_sources(${tgt} PRIVATE ${full_path})
            list(APPEND current_files ${full_path})
        endif()
    endforeach()
endfunction()