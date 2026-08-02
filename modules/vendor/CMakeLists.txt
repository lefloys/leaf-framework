add_library("leaf-vendor" STATIC
    "stb_image.c"
)
add_library("leaf::vendor" ALIAS "leaf-vendor")
target_include_directories("leaf-vendor" PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)
