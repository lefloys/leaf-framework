cmake_minimum_required(VERSION 3.5.0)
project("leaf-framework")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AddFiles)
include(FetchContent)

add_library("leaf-framework" SHARED)
target_compile_definitions("leaf-framework" PRIVATE LEAF_FRAMEWORK_EXPORTS)
set_target_properties("leaf-framework" PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_subdirectory("src")


target_include_directories("leaf-framework" PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_subdirectory("leaf-backend")
add_subdirectory("example")