add_library(leaf-store INTERFACE)
add_library(Leaf::Store ALIAS leaf-store)

target_compile_features(leaf-store INTERFACE cxx_std_23)
target_include_directories(leaf-store INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(leaf-store INTERFACE Leaf::Core)

add_subdirectory("none")

if(WIN32 OR (UNIX AND NOT APPLE))
    add_subdirectory("steam")
endif()
