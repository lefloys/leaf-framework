add_library("leaf-pmg" INTERFACE)
add_library("leaf::pmg" ALIAS "leaf-pmg")
target_include_directories("leaf-pmg" INTERFACE
    "${PROJECT_SOURCE_DIR}/include"
)
