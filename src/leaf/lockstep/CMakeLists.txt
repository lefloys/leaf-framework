add_library(leaf-lockstep STATIC
    session.cpp
)
add_library(Leaf::Lockstep ALIAS leaf-lockstep)

target_compile_features(leaf-lockstep PUBLIC cxx_std_23)
target_include_directories(leaf-lockstep PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(leaf-lockstep PUBLIC Leaf::Core Leaf::Network)
