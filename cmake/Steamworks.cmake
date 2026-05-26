if(TARGET Steamworks::Steamworks)
    return()
endif()

get_filename_component(leaf_steamworks_sdk_dir "${LEAF_STEAMWORKS_SDK_DIR}" ABSOLUTE)
set(LEAF_STEAMWORKS_SDK_DIR "${leaf_steamworks_sdk_dir}" CACHE PATH "Local Steamworks SDK directory" FORCE)

set(steamworks_include_dir "${LEAF_STEAMWORKS_SDK_DIR}/public")
set(steamworks_api_header "${steamworks_include_dir}/steam/steam_api.h")

if(WIN32)
    set(steamworks_import_library "${LEAF_STEAMWORKS_SDK_DIR}/redistributable_bin/win64/steam_api64.lib")
    set(steamworks_runtime_library "${LEAF_STEAMWORKS_SDK_DIR}/redistributable_bin/win64/steam_api64.dll")
elseif(UNIX AND NOT APPLE)
    set(steamworks_import_library "${LEAF_STEAMWORKS_SDK_DIR}/redistributable_bin/linux64/libsteam_api.so")
    set(steamworks_runtime_library "${steamworks_import_library}")
else()
    message(FATAL_ERROR "Steamworks support is only configured for Windows and Linux.")
endif()

foreach(required_file IN ITEMS
    "${steamworks_api_header}"
    "${steamworks_import_library}"
    "${steamworks_runtime_library}"
)
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR
            "Steamworks support was requested, but '${required_file}' is missing. "
            "Copy your private Steamworks SDK into '${LEAF_STEAMWORKS_SDK_DIR}'.")
    endif()
endforeach()

add_library("Steamworks::Steamworks" SHARED IMPORTED GLOBAL)
set_target_properties("Steamworks::Steamworks" PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${steamworks_include_dir}"
    IMPORTED_LOCATION "${steamworks_runtime_library}"
)
if(WIN32)
    set_target_properties("Steamworks::Steamworks" PROPERTIES
        IMPORTED_IMPLIB "${steamworks_import_library}"
    )
endif()

set(LEAF_STEAMWORKS_RUNTIME_LIBRARY "${steamworks_runtime_library}" CACHE FILEPATH "Steamworks runtime library to copy into Steam builds." FORCE)
