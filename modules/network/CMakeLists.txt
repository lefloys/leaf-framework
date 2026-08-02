add_library("leaf-network" STATIC
    "peer.cpp"
    "socket.cpp"
)
add_library("leaf::network" ALIAS "leaf-network")
target_include_directories("leaf-network" PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries("leaf-network" PUBLIC "leaf::core" "leaf::system")
