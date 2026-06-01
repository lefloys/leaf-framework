cmake_minimum_required(VERSION 3.28.0)

if(CMAKE_HOST_WIN32 AND NOT DEFINED VCPKG_TARGET_TRIPLET AND NOT DEFINED CMAKE_VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "vcpkg target triplet")
endif()
if(CMAKE_HOST_WIN32 AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library")
endif()

project("leaf-framework" VERSION 0.1.0 LANGUAGES C CXX)

if(WIN32 AND DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET STREQUAL "x64-windows")
    message(FATAL_ERROR "leaf-framework uses statically linked vcpkg dependencies on Windows. Configure with -DVCPKG_TARGET_TRIPLET=x64-windows-static, or clear the existing build directory if it cached x64-windows.")
endif()
if(WIN32 AND DEFINED CMAKE_VCPKG_TARGET_TRIPLET AND CMAKE_VCPKG_TARGET_TRIPLET STREQUAL "x64-windows")
    message(FATAL_ERROR "leaf-framework uses statically linked vcpkg dependencies on Windows. Configure with -DCMAKE_VCPKG_TARGET_TRIPLET=x64-windows-static, or clear the existing build directory if it cached x64-windows.")
endif()

# Build options.
option(LEAF_BUILD_EXAMPLES "Build leaf-framework examples." ${PROJECT_IS_TOP_LEVEL})
option(LEAF_BUILD_TESTS "Build leaf-framework tests." ${PROJECT_IS_TOP_LEVEL})
option(LEAF_BUILD_TOOLS "Build leaf-framework developer tools." ON)
option(LEAF_BUILD_RUTILE_VULKAN "Build the Vulkan Rutile runtime." ON)
option(LEAF_BUILD_RUTILE_DX12 "Build the DirectX 12 Rutile runtime on Windows." ON)
option(LEAF_BUILD_RUTILE_VALIDATION_LAYER "Build the Rutile validation layer." ON)
option(LEAF_BUILD_RUTILE_LOGGING_LAYER "Build the Rutile logging layer." ON)

# Platform settings.
set(LEAF_SUPPORTED_WINDOW_BACKENDS "glfw")
set(LEAF_WINDOW_BACKEND "glfw" CACHE STRING "Window backend for leaf-framework.")
set_property(CACHE LEAF_WINDOW_BACKEND PROPERTY STRINGS ${LEAF_SUPPORTED_WINDOW_BACKENDS})
set(LEAF_SUPPORTED_STORES "none" "steam")
set(store "none" CACHE STRING "Store backend for leaf-framework.")
set_property(CACHE store PROPERTY STRINGS ${LEAF_SUPPORTED_STORES})

if(NOT "${LEAF_WINDOW_BACKEND}" IN_LIST LEAF_SUPPORTED_WINDOW_BACKENDS)
    list(JOIN LEAF_SUPPORTED_WINDOW_BACKENDS ", " LEAF_SUPPORTED_WINDOW_BACKENDS_TEXT)
    message(FATAL_ERROR "Unsupported LEAF_WINDOW_BACKEND '${LEAF_WINDOW_BACKEND}'. Supported values: ${LEAF_SUPPORTED_WINDOW_BACKENDS_TEXT}.")
endif()
if(NOT "${store}" IN_LIST LEAF_SUPPORTED_STORES)
    list(JOIN LEAF_SUPPORTED_STORES ", " LEAF_SUPPORTED_STORES_TEXT)
    message(FATAL_ERROR "Unsupported store '${store}'. Supported values: ${LEAF_SUPPORTED_STORES_TEXT}.")
endif()

# Shared project helpers.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include("AddFiles")
set(LEAF_STEAMWORKS_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/steam-sdk" CACHE PATH "Local Steamworks SDK directory")
set(LEAF_STEAM_APP_ID "480" CACHE STRING "Steam app id used by the Steam store.")

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
set(LEAF_TARGET_NAME "${PROJECT_NAME}")

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
if(WIN32)
    target_link_libraries("${PROJECT_NAME}" PUBLIC ws2_32)
endif()
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

function(leaf_export_glfw_symbols target_name)
    if(WIN32)
        target_link_options("${target_name}" PRIVATE
            "$<$<CXX_COMPILER_ID:MSVC>:/INCLUDE:glfwCreateWindowSurface>"
            "$<$<CXX_COMPILER_ID:MSVC>:/INCLUDE:glfwGetFramebufferSize>"
            "$<$<CXX_COMPILER_ID:MSVC>:/INCLUDE:glfwGetWin32Window>"
            "$<$<CXX_COMPILER_ID:MSVC>:/EXPORT:glfwCreateWindowSurface>"
            "$<$<CXX_COMPILER_ID:MSVC>:/EXPORT:glfwGetFramebufferSize>"
            "$<$<CXX_COMPILER_ID:MSVC>:/EXPORT:glfwGetWin32Window>"
        )
    else()
        target_link_options("${target_name}" PRIVATE
            "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wl,--undefined=glfwCreateWindowSurface>"
            "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wl,--undefined=glfwGetFramebufferSize>"
            "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wl,--export-dynamic>"
        )
    endif()
endfunction()

function(leaf_configure_executable target_name)
    leaf_configure_store_executable("${target_name}")
    leaf_export_glfw_symbols("${target_name}")
    leaf_copy_rutile_backends("${target_name}")
endfunction()

# Optional developer targets.
if(LEAF_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

if(LEAF_BUILD_TOOLS)
    add_subdirectory("tools")
endif()

if(LEAF_BUILD_TESTS)
    find_package("Catch2" CONFIG REQUIRED)
    add_subdirectory("tests")
endif()
