add_library(leaf-system STATIC
    crash_handler.cpp
)
add_library(Leaf::System ALIAS leaf-system)

target_sources(leaf-system PRIVATE
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/windows/socket.cpp>
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/windows/system.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/posix/socket.cpp>
    $<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/posix/system.cpp>
)

target_compile_features(leaf-system PUBLIC cxx_std_23)
target_include_directories(leaf-system PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(leaf-system PUBLIC Leaf::Core)

if(WIN32)
    target_link_libraries(leaf-system PUBLIC ws2_32)
endif()
