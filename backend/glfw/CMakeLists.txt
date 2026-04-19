include(FetchContent)
find_package(Vulkan REQUIRED)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 3.3.9
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

add_library("leaf-platform-glfw" STATIC
	"include/leaf/platform/backends/glfw.hpp"
	"src/glfw_platform.cpp"
)

target_include_directories("leaf-platform-glfw" PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories("leaf-platform-glfw" PRIVATE
	"${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries("leaf-platform-glfw" PRIVATE
	glfw
	Vulkan::Vulkan
)

target_link_libraries("leaf-framework" PUBLIC
	"leaf-platform-glfw"
)
