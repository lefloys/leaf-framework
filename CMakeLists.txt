cmake_minimum_required(VERSION 4.0.0)

project(leaf-framework VERSION 0.1.0 LANGUAGES C CXX)

# Build options.
option(LEAF_BUILD_EXAMPLES "Build leaf-framework examples." ${PROJECT_IS_TOP_LEVEL})
option(LEAF_BUILD_TESTS "Build leaf-framework tests." ${PROJECT_IS_TOP_LEVEL})
option(LEAF_BUILD_RUTILE_VULKAN "Build the Vulkan Rutile runtime." ON)
option(LEAF_BUILD_RUTILE_DX12 "Build the DirectX 12 Rutile runtime on Windows." ON)
option(LEAF_BUILD_RUTILE_VALIDATION_LAYER "Build the Rutile validation layer." ON)
option(LEAF_BUILD_RUTILE_LOGGING_LAYER "Build the Rutile logging layer." ON)

# Platform settings.
set(LEAF_SUPPORTED_WINDOW_BACKENDS "glfw")
set(LEAF_WINDOW_BACKEND "glfw" CACHE STRING "Window backend for leaf-framework.")
set_property(CACHE LEAF_WINDOW_BACKEND PROPERTY STRINGS ${LEAF_SUPPORTED_WINDOW_BACKENDS})

if(NOT "${LEAF_WINDOW_BACKEND}" IN_LIST LEAF_SUPPORTED_WINDOW_BACKENDS)
    list(JOIN LEAF_SUPPORTED_WINDOW_BACKENDS ", " LEAF_SUPPORTED_WINDOW_BACKENDS_TEXT)
    message(FATAL_ERROR "Unsupported LEAF_WINDOW_BACKEND '${LEAF_WINDOW_BACKEND}'. Supported values: ${LEAF_SUPPORTED_WINDOW_BACKENDS_TEXT}.")
endif()

# Shared project helpers.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include("AddFiles")
set(LEAF_STEAMWORKS_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/steam-sdk" CACHE PATH "Local Steamworks SDK directory")

# Third-party dependencies.
find_package("glfw3" CONFIG REQUIRED)
find_package("glm" CONFIG REQUIRED)
find_package("Stb" REQUIRED)
find_package("RmlUi" CONFIG REQUIRED)
find_package("sol2" CONFIG REQUIRED)
find_package("yaml-cpp" CONFIG REQUIRED)
find_package("Lua" REQUIRED)
find_package("miniaudio" REQUIRED)

# Rutile subproject defaults.
set(RUTILE_BUILD_VULKAN "${LEAF_BUILD_RUTILE_VULKAN}")
set(RUTILE_BUILD_DX12 "${LEAF_BUILD_RUTILE_DX12}")
set(RUTILE_BUILD_VALIDATION_LAYER "${LEAF_BUILD_RUTILE_VALIDATION_LAYER}")
set(RUTILE_BUILD_LOGGING_LAYER "${LEAF_BUILD_RUTILE_LOGGING_LAYER}")
if(NOT TARGET "Rutile::rutile")
    add_subdirectory("Rutile")
endif()

# Framework target.
add_library("${PROJECT_NAME}" STATIC)

target_compile_features("${PROJECT_NAME}" PUBLIC "cxx_std_23")
target_compile_definitions("${PROJECT_NAME}" PRIVATE LEAF_WINDOW_BACKEND_GLFW)
target_include_directories("${PROJECT_NAME}" PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")
target_include_directories("${PROJECT_NAME}" PUBLIC "${Stb_INCLUDE_DIR}" "${LUA_INCLUDE_DIR}")

target_link_libraries("${PROJECT_NAME}"
    PUBLIC
        glfw
        glm::glm
        RmlUi::RmlUi
        Rutile::rutile
        miniaudio::miniaudio
        sol2::sol2
        yaml-cpp::yaml-cpp
        ${LUA_LIBRARIES}
)
add_subdirectory(src)

function(leaf_copy_rutile_backends target_name)
    if(NOT COMMAND rutile_get_runtime_targets)
        return()
    endif()

    rutile_get_runtime_targets(rutile_backends)
    foreach(rutile_backend IN LISTS rutile_backends)
        add_dependencies("${target_name}" "${rutile_backend}")
        add_custom_command(TARGET "${target_name}" POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${rutile_backend}>"
                "$<TARGET_FILE_DIR:${target_name}>"
        )
    endforeach()
endfunction()

function(leaf_configure_executable target_name launcher_kind steam_app_id)
    set(leaf_launcher_source_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/leaf/launcher")
    if("${launcher_kind}" STREQUAL "none")
        target_sources("${target_name}" PRIVATE "${leaf_launcher_source_dir}/none/launcher.cpp")
    elseif("${launcher_kind}" STREQUAL "steam")
        include("Steamworks")
        target_sources("${target_name}" PRIVATE "${leaf_launcher_source_dir}/steam/launcher.cpp")
        target_link_libraries("${target_name}" PRIVATE "Steamworks::Steamworks")
        file(GENERATE
            OUTPUT "$<TARGET_FILE_DIR:${target_name}>/steam_appid.txt"
            CONTENT "${steam_app_id}\n"
        )
        add_custom_target("${target_name}-steam-runtime"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${target_name}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LEAF_STEAMWORKS_RUNTIME_LIBRARY}"
                "$<TARGET_FILE_DIR:${target_name}>"
        )
        add_dependencies("${target_name}" "${target_name}-steam-runtime")
    else()
        message(FATAL_ERROR "Unsupported launcher kind '${launcher_kind}'. Expected 'none' or 'steam'.")
    endif()
    leaf_copy_rutile_backends("${target_name}")
endfunction()

# Optional developer targets.
if(LEAF_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

if(LEAF_BUILD_TESTS)
    find_package("Catch2" CONFIG REQUIRED)
    add_subdirectory("tests")
endif()
