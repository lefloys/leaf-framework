add_library("leaf-store" INTERFACE)
add_library("leaf::store" ALIAS "leaf-store")
target_include_directories("leaf-store" INTERFACE
    "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries("leaf-store" INTERFACE "leaf::core")

add_subdirectory("none")

if(WIN32 OR (UNIX AND NOT APPLE))
    add_subdirectory("steam")
endif()
