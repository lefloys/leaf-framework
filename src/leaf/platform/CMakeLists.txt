add_library(leaf-platform STATIC
    desktop_glfw.cpp
)
add_library(Leaf::Platform ALIAS leaf-platform)

target_compile_features(leaf-platform PUBLIC cxx_std_23)
target_include_directories(leaf-platform PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_compile_definitions(leaf-platform PRIVATE LEAF_WINDOW_BACKEND_GLFW)
target_link_libraries(leaf-platform
    PUBLIC
        Leaf::Core
        Leaf::System
        glfw
)
