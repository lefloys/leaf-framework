cmake_minimum_required(VERSION 3.24)

project(Leaf VERSION 0.1.0 LANGUAGES C CXX)

option(LEAF_BUILD_EXAMPLES "Build Leaf examples." ON)
option(LEAF_BUILD_TESTS "Build Leaf tests." ON)
option(LEAF_BUILD_RUTILE_RUNTIMES "Build Rutile runtimes when Leaf consumes a Rutile source checkout." ON)
set(LEAF_WINDOW_BACKEND "glfw" CACHE STRING "Leaf platform window backend.")
set_property(CACHE LEAF_WINDOW_BACKEND PROPERTY STRINGS glfw)

if(NOT LEAF_WINDOW_BACKEND STREQUAL "glfw")
    message(FATAL_ERROR "Leaf currently only supports the glfw window backend because Rutile presentation is GLFW-only.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AddFiles)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(RmlUi CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Lua REQUIRED)

if(NOT TARGET Lua::Lua)
    add_library(Lua::Lua INTERFACE IMPORTED)
    set_target_properties(Lua::Lua PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
    )
    target_link_libraries(Lua::Lua INTERFACE ${LUA_LIBRARIES})
endif()

set(RUTILE_BUILD_EXAMPLES "${LEAF_BUILD_EXAMPLES}" CACHE BOOL "Build Rutile examples." FORCE)
set(RUTILE_BUILD_VULKAN "${LEAF_BUILD_RUTILE_RUNTIMES}" CACHE BOOL "Build the Vulkan Rutile runtime." FORCE)
set(RUTILE_BUILD_DX12 "${LEAF_BUILD_RUTILE_RUNTIMES}" CACHE BOOL "Build the DirectX 12 Rutile runtime." FORCE)
set(RUTILE_BUILD_VALIDATION_LAYER "${LEAF_BUILD_RUTILE_RUNTIMES}" CACHE BOOL "Build the Rutile validation layer." FORCE)
set(RUTILE_BUILD_LOGGING_LAYER "${LEAF_BUILD_RUTILE_RUNTIMES}" CACHE BOOL "Build the Rutile logging layer." FORCE)
add_subdirectory("Rutile")

add_library(leaf STATIC)

target_compile_features(leaf PUBLIC cxx_std_23)
target_compile_definitions(leaf PRIVATE LEAF_WINDOW_BACKEND_GLFW)
target_include_directories(leaf
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
)
target_link_libraries(leaf
    PUBLIC
        glfw
        glm::glm
        RmlUi::RmlUi
        Rutile::rutile
    PRIVATE
        sol2::sol2
        yaml-cpp::yaml-cpp
        Lua::Lua
)
target_include_directories(leaf PRIVATE ${Stb_INCLUDE_DIR})

add_subdirectory(src)

if(LEAF_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

if(LEAF_BUILD_TESTS)
    find_package(Catch2 CONFIG REQUIRED)
    add_subdirectory(tests)
endif()
