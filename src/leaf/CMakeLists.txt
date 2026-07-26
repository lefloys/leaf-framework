add_library(leaf-framework STATIC
    "leaf.cpp"
    "leaf.hpp"
    "config.hpp"
)
add_library(Leaf::Framework ALIAS leaf-framework)

target_compile_features(leaf-framework PUBLIC cxx_std_23)
target_include_directories(leaf-framework PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:include>"
)

add_subdirectory("core")
add_subdirectory("math")
add_subdirectory("system")
add_subdirectory("platform")
add_subdirectory("network")
add_subdirectory("lockstep")
add_subdirectory("script")
add_subdirectory("store")
add_subdirectory("graphics")
add_subdirectory("application")
add_subdirectory("pmg")
add_subdirectory("vendor")

target_link_libraries(leaf-framework PUBLIC
    Leaf::Application
    Leaf::Lockstep
)
