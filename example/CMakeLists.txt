function(leaf_configure_example target_name)
    target_compile_features(${target_name} PRIVATE cxx_std_23)
    target_link_libraries(${target_name} PRIVATE leaf)

    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target_name}>"
    )

    if(CMAKE_CONFIGURATION_TYPES)
        foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER "${cfg}" cfg_upper)
            set_target_properties(${target_name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_${cfg_upper} "${CMAKE_BINARY_DIR}/bin/${cfg}"
            )
        endforeach()
    endif()

    if(COMMAND rutile_copy_runtime_dependencies)
        rutile_copy_runtime_dependencies(${target_name})
    endif()
endfunction()

add_executable(leaf-core-example)
add_files(leaf-core-example
    "core/main.cpp"
)
leaf_configure_example(leaf-core-example)
