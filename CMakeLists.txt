cmake_minimum_required(VERSION 3.28)
project(Leaf VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
include(CTest)

set(LEAF_STEAMWORKS_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/steam-sdk" CACHE PATH "Steamworks SDK directory")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddFiles.cmake")
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(unofficial-lua CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(RmlUi CONFIG REQUIRED)
find_package(miniaudio REQUIRED)

add_subdirectory("Rutile")

add_subdirectory("src")

add_subdirectory("example" EXCLUDE_FROM_ALL)
add_subdirectory("tools" EXCLUDE_FROM_ALL)
if(BUILD_TESTING)
    include(CTest)
    find_package(Catch2 CONFIG REQUIRED)
    add_subdirectory("tests" EXCLUDE_FROM_ALL)
endif()
