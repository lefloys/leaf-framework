cmake_minimum_required(VERSION 3.5.0)
project("leaf-framework-example")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AddFiles)
include(FetchContent)

add_executable("leaf-framework-example")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src")

target_include_directories("leaf-framework-example" PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries("leaf-framework-example" PUBLIC "leaf-framework")
