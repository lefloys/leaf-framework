find_path(Stb_INCLUDE_DIR
    NAMES stb_image.h
    PATH_SUFFIXES include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Stb REQUIRED_VARS Stb_INCLUDE_DIR)

if(Stb_FOUND AND NOT TARGET Stb::Stb)
    add_library(Stb::Stb INTERFACE IMPORTED)
    set_target_properties(Stb::Stb PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Stb_INCLUDE_DIR}"
    )
endif()
