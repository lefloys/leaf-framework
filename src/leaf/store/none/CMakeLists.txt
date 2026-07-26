add_library(leaf-store-standalone STATIC
    lifecycle.cpp
    network.cpp
    user.cpp
)
add_library(Leaf::StoreStandalone ALIAS leaf-store-standalone)

target_compile_features(leaf-store-standalone PUBLIC cxx_std_23)
target_include_directories(leaf-store-standalone PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(leaf-store-standalone PUBLIC Leaf::Store)
