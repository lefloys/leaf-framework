add_library(leaf-core STATIC
    dynamic_object.cpp
    error.cpp
    exception.cpp
    filesystem.cpp
    fixed.cpp
    logging.cpp
    messages.cpp
    profiler.cpp
    string.cpp
    time.cpp
)
add_library(Leaf::Core ALIAS leaf-core)

target_compile_features(leaf-core PUBLIC cxx_std_23)
target_include_directories(leaf-core PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(leaf-core
    PUBLIC
        glm::glm
        Stb::Stb
        Rutile::rutile
    PRIVATE
        yaml-cpp::yaml-cpp
)
