cmake_minimum_required(VERSION 3.5.0)
if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()
project("leaf-framework")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AddFiles)
include(FetchContent)

add_library("leaf-framework")
add_subdirectory("src")
add_subdirectory("backend")


target_include_directories("leaf-framework" PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_subdirectory("example")

if(BUILD_TESTING)
    add_subdirectory("tests")
endif()
