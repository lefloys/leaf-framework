cmake_minimum_required(VERSION 3.5.0)
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()
project("leaf-framework")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)

option(LEAF_DEBUG "Build leaf with Vulkan validation/debug support enabled" OFF)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AddFiles)
include(FetchContent)

add_library("leaf-framework")
if(LEAF_DEBUG)
    target_compile_definitions("leaf-framework" PUBLIC LEAF_DEBUG=1)
endif()
add_subdirectory("src")
target_include_directories("leaf-framework" PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_subdirectory("backend/glfw")
add_subdirectory("backend/vulkan")

add_subdirectory("example")

if(BUILD_TESTING)
    add_subdirectory("tests")
endif()

function(add_leaf_variant_target target_name build_dir)
    set(configure_args
        -S "${CMAKE_SOURCE_DIR}"
        -B "${build_dir}"
        -DLEAF_DEBUG=${LEAF_DEBUG}
    )

    if(CMAKE_GENERATOR)
        list(APPEND configure_args -G "${CMAKE_GENERATOR}")
    endif()
    if(CMAKE_GENERATOR_PLATFORM)
        list(APPEND configure_args -A "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
        list(APPEND configure_args -T "${CMAKE_GENERATOR_TOOLSET}")
    endif()

    foreach(extra_arg IN LISTS ARGN)
        list(APPEND configure_args "${extra_arg}")
    endforeach()

    add_custom_target("${target_name}"
        COMMAND "${CMAKE_COMMAND}" ${configure_args}
        COMMAND "${CMAKE_COMMAND}" --build "${build_dir}" --config $<CONFIG>
        USES_TERMINAL
    )
endfunction()

add_leaf_variant_target("leaf-debug" "${CMAKE_SOURCE_DIR}/out/build-debug"
    -DLEAF_DEBUG=ON
)
