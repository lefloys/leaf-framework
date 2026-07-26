foreach(required_var IN ITEMS BUILD_DIR STAGE_DIR PACKAGE_FILE COMPONENT)
    if(NOT DEFINED ${required_var} OR "${${required_var}}" STREQUAL "")
        message(FATAL_ERROR "${required_var} is required")
    endif()
endforeach()

file(REMOVE_RECURSE "${STAGE_DIR}")
file(MAKE_DIRECTORY "${STAGE_DIR}")
get_filename_component(_package_dir "${PACKAGE_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${_package_dir}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}"
        --component "${COMPONENT}"
        --prefix "${STAGE_DIR}"
    RESULT_VARIABLE _install_result
    COMMAND_ECHO STDOUT
)
if(NOT _install_result EQUAL 0)
    message(FATAL_ERROR "Installation of component '${COMPONENT}' failed")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${PACKAGE_FILE}" --format=zip -- .
    WORKING_DIRECTORY "${STAGE_DIR}"
    RESULT_VARIABLE _archive_result
    COMMAND_ECHO STDOUT
)
if(NOT _archive_result EQUAL 0)
    message(FATAL_ERROR "Creating '${PACKAGE_FILE}' failed")
endif()
