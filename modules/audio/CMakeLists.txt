find_path("MINIAUDIO_INCLUDE_DIR" "miniaudio.h" REQUIRED)

add_library("leaf-audio" STATIC "sound.cpp")
add_library("leaf::audio" ALIAS "leaf-audio")
target_include_directories("leaf-audio" PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories("leaf-audio" PRIVATE "${MINIAUDIO_INCLUDE_DIR}")
target_link_libraries("leaf-audio" PUBLIC "leaf::core")
