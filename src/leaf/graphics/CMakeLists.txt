add_library(leaf-graphics STATIC
    buffer.cpp
    command_buffer.cpp
    framebuffer.cpp
    graphics.cpp
    graphics_program.cpp
    queue.cpp
    resource.cpp
    rutile.c
    texture.cpp
    texture_atlas.cpp
    texture_view.cpp
    timepoint.cpp
    window.cpp
)
add_library(Leaf::Graphics ALIAS leaf-graphics)

target_compile_features(leaf-graphics PUBLIC cxx_std_23)
target_include_directories(leaf-graphics PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(leaf-graphics
    PUBLIC
        Leaf::Core
        Leaf::Platform
        Rutile::rutile
)
