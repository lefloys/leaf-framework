add_library(leaf-network STATIC
    peer.cpp
    socket.cpp
)
add_library(Leaf::Network ALIAS leaf-network)

target_compile_features(leaf-network PUBLIC cxx_std_23)
target_include_directories(leaf-network PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(leaf-network PUBLIC Leaf::Core Leaf::System)
