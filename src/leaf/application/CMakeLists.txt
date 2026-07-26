add_library(leaf-application STATIC
    cursor.cpp
    rml_backend.cpp
    scene.cpp
    sound.cpp
    elements/window.cpp
    rml_backend.rtsl
    "${PROJECT_SOURCE_DIR}/src/leaf/vendor/stb_image.c"
)
add_library(Leaf::Application ALIAS leaf-application)

set_source_files_properties(rml_backend.rtsl PROPERTIES HEADER_FILE_ONLY TRUE)

target_compile_features(leaf-application PUBLIC cxx_std_23)
target_include_directories(leaf-application PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(leaf-application
    PUBLIC
        Leaf::Core
        Leaf::System
        Leaf::Platform
        Leaf::Graphics
        Leaf::Network
        Leaf::Script
        rtsl-sdk
        RmlUi::RmlUi
        miniaudio::miniaudio
)

rtsl_add_program(leaf-rml-rtsl
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/rtsl"
    SYMBOL leaf_rml_ui_rtslp
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/rml_backend.rtsl"
)
rtsl_embed_program(leaf-application PROGRAMS leaf-rml-rtsl)
