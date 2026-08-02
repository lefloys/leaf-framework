find_path("STB_INCLUDE_DIR" "stb_image.h" REQUIRED)

add_library("leaf-graphics" STATIC
    "buffer.cpp"
    "command_buffer.cpp"
    "cursor.cpp"
    "framebuffer.cpp"
    "graphics.cpp"
    "graphics_program.cpp"
    "queue.cpp"
    "resource.cpp"
    "rutile.c"
    "texture.cpp"
    "texture_atlas.cpp"
    "texture_view.cpp"
    "timepoint.cpp"
    "window.cpp"
)
add_library("leaf::graphics" ALIAS "leaf-graphics")
target_include_directories("leaf-graphics" PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)
target_include_directories("leaf-graphics" PRIVATE "${STB_INCLUDE_DIR}")
target_link_libraries("leaf-graphics"
    PUBLIC
        "leaf::core"
        "leaf::platform"
        "Rutile::rutile"
)
