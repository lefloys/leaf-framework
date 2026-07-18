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
option(LEAF_BUILD_RUTILE_VK13 "Build the Vulkan 1.3 Rutile runtime." ON)
option(LEAF_BUILD_RUTILE_DX12 "Build the DirectX 12 Rutile runtime on Windows." ON)
option(LEAF_BUILD_RUTILE_GL33 "Build the OpenGL 3.3 Rutile runtime." OFF)
option(LEAF_BUILD_RUTILE_VALIDATION_LAYER "Build the Rutile validation layer." ON)
option(LEAF_BUILD_RUTILE_LOGGING_LAYER "Build the Rutile logging layer." ON)
option(LEAF_HEADLESS "Build Leaf without headed window/application support." OFF)

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

function(leaf_clear_embedded_stale_package_cache cache_name)
    if(PROJECT_IS_TOP_LEVEL OR NOT DEFINED CACHE{${cache_name}})
        return()
    endif()
    get_property(cache_value CACHE "${cache_name}" PROPERTY VALUE)
    cmake_path(IS_PREFIX CMAKE_CURRENT_SOURCE_DIR "${cache_value}" NORMALIZE is_leaf_local_cache)
    if(is_leaf_local_cache)
        unset("${cache_name}" CACHE)
    endif()
endfunction()

leaf_clear_embedded_stale_package_cache(glm_DIR)
leaf_clear_embedded_stale_package_cache(sol2_DIR)
leaf_clear_embedded_stale_package_cache(yaml-cpp_DIR)
leaf_clear_embedded_stale_package_cache(Stb_INCLUDE_DIR)

# Shared project helpers.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include("AddFiles")
set(LEAF_STEAMWORKS_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/steam-sdk" CACHE PATH "Local Steamworks SDK directory")
set(LEAF_STEAM_APP_ID "480" CACHE STRING "Steam app id used by the Steam store.")

# Third-party dependencies.
find_package("glm" CONFIG REQUIRED)
find_package("Stb" REQUIRED)
find_package("sol2" CONFIG REQUIRED)
find_package("yaml-cpp" CONFIG REQUIRED)
find_package("unofficial-lua" CONFIG REQUIRED)
if(NOT LEAF_HEADLESS)
    find_package("glfw3" CONFIG REQUIRED)
    find_package("RmlUi" CONFIG REQUIRED)
    find_package("miniaudio" REQUIRED)
endif()

# Rutile subproject defaults.
set(RUTILE_BUILD_VK13 "${LEAF_BUILD_RUTILE_VK13}")
set(RUTILE_BUILD_DX12 "${LEAF_BUILD_RUTILE_DX12}")
set(RUTILE_BUILD_GL33 "${LEAF_BUILD_RUTILE_GL33}")
set(RUTILE_BUILD_VALIDATION_LAYER "${LEAF_BUILD_RUTILE_VALIDATION_LAYER}")
set(RUTILE_BUILD_LOGGING_LAYER "${LEAF_BUILD_RUTILE_LOGGING_LAYER}")
set(RUTILE_BUILD_EXAMPLES "${LEAF_BUILD_EXAMPLES}")
if(NOT TARGET "Rutile::rutile")
    add_subdirectory("Rutile")
endif()

# Framework target.
add_library("${PROJECT_NAME}" STATIC)
set(LEAF_TARGET_NAME "${PROJECT_NAME}")

target_compile_features("${PROJECT_NAME}" PUBLIC "cxx_std_23")
target_compile_definitions("${PROJECT_NAME}" PUBLIC "LF_HEADLESS=$<BOOL:${LEAF_HEADLESS}>")
if(NOT LEAF_HEADLESS)
    target_compile_definitions("${PROJECT_NAME}" PRIVATE LEAF_WINDOW_BACKEND_GLFW)
endif()
target_include_directories("${PROJECT_NAME}" PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")
target_include_directories("${PROJECT_NAME}" PUBLIC "${Stb_INCLUDE_DIR}")

target_link_libraries("${PROJECT_NAME}"
    PUBLIC
        glm::glm
        Rutile::rutile
        sol2::sol2
    PRIVATE
        yaml-cpp::yaml-cpp
        unofficial::lua::lua
)
if(NOT LEAF_HEADLESS)
    target_link_libraries("${PROJECT_NAME}"
        PUBLIC
            glfw
            RmlUi::RmlUi
            miniaudio::miniaudio
    )
endif()
if(WIN32)
    target_link_libraries("${PROJECT_NAME}" PUBLIC ws2_32)
endif()

set(LEAF_RML_RTSL "${CMAKE_CURRENT_SOURCE_DIR}/src/leaf/application/rml_backend.rtsl")
set_source_files_properties("${LEAF_RML_RTSL}" PROPERTIES HEADER_FILE_ONLY TRUE)
target_sources("${PROJECT_NAME}" PRIVATE "${LEAF_RML_RTSL}")
rtsl_add_program("${PROJECT_NAME}"
    RTSLC rtslc
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/rtsl"
    EMBED
    EMBED_NAME leaf_rml_ui_rtslp
    SOURCES "${LEAF_RML_RTSL}"
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

function(leaf_export_glfw_symbols target_name)
    if(LEAF_HEADLESS)
        return()
    endif()
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
    if(NOT LEAF_HEADLESS)
        leaf_configure_store_executable("${target_name}")
    endif()
    if(MSVC)
        set_property(TARGET "${target_name}" APPEND_STRING PROPERTY LINK_FLAGS " /NODEFAULTLIB:lua.lib /NODEFAULTLIB:yaml-cpp.lib /FORCE:MULTIPLE")
    endif()
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
