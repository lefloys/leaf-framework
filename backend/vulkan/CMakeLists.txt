find_package(Vulkan REQUIRED)

add_library("leaf-graphics-vulkan" STATIC)

add_subdirectory("src")
add_files("leaf-graphics-vulkan"
	"include/leaf/graphics/backends/vulkan.hpp"
)

target_include_directories("leaf-graphics-vulkan" PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories("leaf-graphics-vulkan" PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries("leaf-graphics-vulkan" PUBLIC
	"leaf-framework"
	Vulkan::Vulkan
)
