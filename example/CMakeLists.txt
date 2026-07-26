function(leaf_framework_configure_example target_name)
    target_compile_features("${target_name}" PRIVATE cxx_std_23)

    get_target_property(store_backend "${target_name}" LEAF_STORE_BACKEND)
    if(store_backend STREQUAL "steam")
        if(NOT TARGET Leaf::StoreSteam)
            message(FATAL_ERROR "${target_name} requests the Steam store, but Steam support is unavailable")
        endif()
        target_link_libraries("${target_name}" PRIVATE Leaf::StoreSteam)
    else()
        target_link_libraries("${target_name}" PRIVATE Leaf::StoreStandalone)
    endif()
    target_link_libraries("${target_name}" PRIVATE Leaf::Framework)

    set(example_runtimes)
    foreach(runtime_target IN ITEMS
        rt-vulkan
        rt-opengl
        rt-directx12
        rt-validation-layer
        rt-logging-layer
    )
        if(TARGET "${runtime_target}")
            list(APPEND example_runtimes "${runtime_target}")
        endif()
    endforeach()
    if(example_runtimes)
        rutile_use_runtime("${target_name}" ${example_runtimes})
    endif()

    set_target_properties("${target_name}" PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target_name}>"
    )
endfunction()

add_subdirectory(core)
add_subdirectory(network)
add_subdirectory(steam)
