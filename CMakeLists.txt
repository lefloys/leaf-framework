cmake_minimum_required(VERSION 3.24)

project(leaf-framework VERSION 0.1.0 LANGUAGES C CXX)

# Build options.
option(LEAF_BUILD_EXAMPLES "Build leaf-framework examples." ${PROJECT_IS_TOP_LEVEL})
option(LEAF_BUILD_TESTS "Build leaf-framework tests." ${PROJECT_IS_TOP_LEVEL})
option(LEAF_BUILD_RUTILE_VULKAN "Build the Vulkan Rutile runtime." ON)
option(LEAF_BUILD_RUTILE_DX12 "Build the DirectX 12 Rutile runtime on Windows." ON)
option(LEAF_BUILD_RUTILE_VALIDATION_LAYER "Build the Rutile validation layer." ON)
option(LEAF_BUILD_RUTILE_LOGGING_LAYER "Build the Rutile logging layer." ON)

# Platform settings.
set(LEAF_SUPPORTED_WINDOW_BACKENDS glfw)
set(LEAF_WINDOW_BACKEND "glfw" CACHE STRING "Window backend for leaf-framework.")
set_property(CACHE LEAF_WINDOW_BACKEND PROPERTY STRINGS ${LEAF_SUPPORTED_WINDOW_BACKENDS})

if(NOT LEAF_WINDOW_BACKEND IN_LIST LEAF_SUPPORTED_WINDOW_BACKENDS)
    list(JOIN LEAF_SUPPORTED_WINDOW_BACKENDS ", " LEAF_SUPPORTED_WINDOW_BACKENDS_TEXT)
    message(FATAL_ERROR "Unsupported LEAF_WINDOW_BACKEND '${LEAF_WINDOW_BACKEND}'. Supported values: ${LEAF_SUPPORTED_WINDOW_BACKENDS_TEXT}.")
endif()

# Shared project helpers.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AddFiles)

# Third-party dependencies.
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(RmlUi CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Lua REQUIRED)

# Rutile subproject defaults.
set(RUTILE_BUILD_VULKAN "${LEAF_BUILD_RUTILE_VULKAN}" CACHE BOOL "Build the Vulkan Rutile runtime.")
set(RUTILE_BUILD_DX12 "${LEAF_BUILD_RUTILE_DX12}" CACHE BOOL "Build the DirectX 12 Rutile runtime on Windows.")
set(RUTILE_BUILD_VALIDATION_LAYER "${LEAF_BUILD_RUTILE_VALIDATION_LAYER}" CACHE BOOL "Build the Rutile validation layer.")
set(RUTILE_BUILD_LOGGING_LAYER "${LEAF_BUILD_RUTILE_LOGGING_LAYER}" CACHE BOOL "Build the Rutile logging layer.")
add_subdirectory("Rutile")

# Framework target.
add_library(${PROJECT_NAME} STATIC)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PRIVATE LEAF_WINDOW_BACKEND_GLFW)
target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")
target_include_directories(${PROJECT_NAME} PUBLIC ${Stb_INCLUDE_DIR} ${LUA_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        glfw
        glm::glm
        RmlUi::RmlUi
        Rutile::rutile
        sol2::sol2
        yaml-cpp::yaml-cpp
        ${LUA_LIBRARIES}
)

add_subdirectory(src)

# Optional developer targets.
if(LEAF_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

if(LEAF_BUILD_TESTS)
    find_package(Catch2 CONFIG REQUIRED)
    add_subdirectory(tests)
endif()

add_custom_target(leaf-framework-dev)
if(LEAF_BUILD_EXAMPLES)
    add_dependencies(leaf-framework-dev leaf-framework-core-example)
endif()
if(LEAF_BUILD_TESTS)
    add_dependencies(leaf-framework-dev leaf-framework-tests)
endif()
