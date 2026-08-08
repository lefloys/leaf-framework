cmake_minimum_required(VERSION 3.28)
project(Leaf VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_subdirectory("Rutile")

add_subdirectory("modules")

include(CTest)
if(BUILD_TESTING)
    add_subdirectory("tests")
endif()

