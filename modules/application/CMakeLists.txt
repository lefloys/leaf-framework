find_package("ZLIB" REQUIRED)
find_package("Freetype" CONFIG REQUIRED)
find_package("RmlUi" CONFIG REQUIRED)
find_path("STB_INCLUDE_DIR" "stb_image.h" REQUIRED)
find_path("MINIAUDIO_INCLUDE_DIR" "miniaudio.h" REQUIRED)

add_library("leaf-application" STATIC
    "rml_backend.cpp"
    "scene.cpp"
    "scene_systems.cpp"
	"window.cpp"
	"window_cursor.cpp"
    "rml_backend.rtsl"
)
add_library("leaf::application" ALIAS "leaf-application")

if(CMAKE_CONFIGURATION_TYPES)
    set(LEAF_APPLICATION_RESOURCE_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/leaf/application")
else()
    set(LEAF_APPLICATION_RESOURCE_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/leaf/application")
endif()
add_custom_target("leaf-application-assets"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${LEAF_APPLICATION_RESOURCE_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/window.rcss"
        "${LEAF_APPLICATION_RESOURCE_DIR}/window.rcss"
    VERBATIM
)
add_dependencies("leaf-application" "leaf-application-assets")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/window.rcss" DESTINATION "leaf/application")

set_source_files_properties("rml_backend.rtsl" PROPERTIES HEADER_FILE_ONLY TRUE)
target_include_directories("leaf-application" PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)
target_include_directories("leaf-application" PRIVATE
    "${STB_INCLUDE_DIR}"
    "${MINIAUDIO_INCLUDE_DIR}"
)
target_link_libraries("leaf-application"
    PUBLIC
        "leaf::core"
        "leaf::system"
        "leaf::platform"
        "leaf::graphics"
        "leaf::network"
        "leaf::script"
        "leaf::audio"
        "leaf::vendor"
        "rtsl-sdk"
        "RmlUi::RmlUi"
)

rtsl_add_program("leaf-rml-rtsl"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/rtsl"
    SYMBOL leaf_rml_ui_rtslp
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/rml_backend.rtsl"
)
rtsl_embed_program("leaf-application" PROGRAMS "leaf-rml-rtsl")
