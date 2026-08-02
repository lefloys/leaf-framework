add_library("leaf-resource" INTERFACE)
add_library("leaf::resource" ALIAS "leaf-resource")
target_include_directories("leaf-resource" INTERFACE
    "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries("leaf-resource" INTERFACE
    "leaf::core"
)
