add_library("leaf-math" INTERFACE)
add_library("leaf::math" ALIAS "leaf-math")
target_include_directories("leaf-math" INTERFACE
    "${PROJECT_SOURCE_DIR}/include"
)
