find_package(Vulkan REQUIRED)

project("leaf-framework-graphics-vulkan" LANGUAGES CXX)

add_library(${PROJECT_NAME} SHARED)

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_subdirectory("src")
find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Vulkan::Vulkan
)
target_link_libraries(${PROJECT_NAME} PRIVATE "leaf-framework")
