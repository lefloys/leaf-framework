if(NOT EXISTS "${LEAF_STEAMWORKS_SDK_DIR}/public/steam/steam_api.h")
    message(FATAL_ERROR "LEAF_STEAMWORKS_SDK_DIR is invalid: ${LEAF_STEAMWORKS_SDK_DIR}")
endif()

add_library(leaf-store-steam STATIC
    lifecycle.cpp
    network.cpp
    user.cpp
)
add_library(Leaf::StoreSteam ALIAS leaf-store-steam)

target_compile_features(leaf-store-steam PUBLIC cxx_std_23)
target_include_directories(leaf-store-steam
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    PRIVATE "${LEAF_STEAMWORKS_SDK_DIR}/public"
)
target_link_libraries(leaf-store-steam PUBLIC Leaf::Store)

if(WIN32)
    target_link_libraries(leaf-store-steam PRIVATE
        "${LEAF_STEAMWORKS_SDK_DIR}/redistributable_bin/win64/steam_api64.lib"
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(leaf-store-steam PRIVATE
        "${LEAF_STEAMWORKS_SDK_DIR}/redistributable_bin/linux64/libsteam_api.so"
    )
endif()
