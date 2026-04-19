find_package(Vulkan REQUIRED)

target_include_directories("leaf-framework" PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories("leaf-framework" PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries("leaf-framework" PUBLIC Vulkan::Vulkan)

add_files("leaf-framework"
	"include/leaf/graphics/backends/vulkan.hpp"
	"src/object_allocator.hpp"
	"src/vulkan_context.hpp"
	"src/vulkan_context.cpp"
	"src/vulkan_backend.cpp"
	"src/resources/resource.hpp"
	"src/resources/window_vk.hpp"
	"src/resources/window_vk.cpp"
)
