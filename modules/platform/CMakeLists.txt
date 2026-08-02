find_package("glfw3" CONFIG REQUIRED)

add_library("leaf-platform" STATIC
    "desktop_glfw.cpp"
)
add_library("leaf::platform" ALIAS "leaf-platform")
target_include_directories("leaf-platform" PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)
target_compile_definitions("leaf-platform" PRIVATE LEAF_WINDOW_BACKEND_GLFW)
target_link_libraries("leaf-platform"
    PUBLIC
        "leaf::core"
        "leaf::system"
        "glfw"
)
