add_library(leaf-script STATIC
    localization.cpp
    mod_enabled.cpp
    mod_info.cpp
    mod_loader.cpp
    prototype_inspector_script.cpp
    virtual_filesystem.cpp
    prototypes/cursor.cpp
    prototypes/sound.cpp
    prototypes/texture.cpp
)
add_library(Leaf::Script ALIAS leaf-script)

target_compile_features(leaf-script PUBLIC cxx_std_23)
target_include_directories(leaf-script PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(leaf-script
    PUBLIC
        Leaf::Core
        sol2::sol2
    PRIVATE
        lua
)
