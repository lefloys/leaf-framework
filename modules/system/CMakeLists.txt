add_library("leaf-system" STATIC
    "crash_handler.cpp"
)
add_library("leaf::system" ALIAS "leaf-system")

target_sources("leaf-system" PRIVATE
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/windows/socket.cpp>
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/windows/system.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/posix/socket.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/posix/system.cpp>
)
target_include_directories("leaf-system" PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries("leaf-system" PUBLIC "leaf::core")

if(WIN32)
    target_link_libraries("leaf-system" PUBLIC ws2_32)
endif()
