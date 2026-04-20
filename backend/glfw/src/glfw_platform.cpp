#include <leaf/platform/backends/glfw.hpp>

#include "leaf/core/cstddef.hpp"
#include "leaf/core/exception.hpp"
#include "leaf/core/vector.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


lf::platform_window* to_platform_window(GLFWwindow* wnd) {
	return reinterpret_cast<lf::platform_window*>(wnd);
}

GLFWwindow* to_glfw_window(lf::platform_window* wnd) {
	return reinterpret_cast<GLFWwindow*>(wnd);
}

const GLFWwindow* to_glfw_window(const lf::platform_window* wnd) {
	return reinterpret_cast<const GLFWwindow*>(wnd);
}

lf::error platform_init() {
	if (!glfwInit()) {
		return lf::error(lf::generic_errc::unknown, "failed to initialize GLFW");
	}

	return lf::error::no_error;
}

void platform_exit() {
	glfwTerminate();
}

bool platform_vulkan_supported() {
	return glfwVulkanSupported();
}

const char** get_platform_vulkan_instance_extensions(u32& extension_count) {
	return glfwGetRequiredInstanceExtensions(&extension_count);
}

lf::platform_window* create_platform_window(lf::string_view title, lf::dim2<i32> extent) {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	GLFWwindow* glfw_window = glfwCreateWindow(extent.width, extent.height, title.data(), nullptr, nullptr);
	if (!glfw_window) {
		const char* description = nullptr;
		glfwGetError(&description);
		throw lf::runtime_exception(description ? description : "failed to create GLFW window");
	}

	return to_platform_window(glfw_window);
}

void destroy_platform_window(lf::platform_window* wnd) {
	glfwDestroyWindow(to_glfw_window(wnd));
}

void show_platform_window(lf::platform_window* wnd) {
	glfwShowWindow(to_glfw_window(wnd));
}

void hide_platform_window(lf::platform_window* wnd) {
	glfwHideWindow(to_glfw_window(wnd));
}

void set_platform_window_extent(lf::platform_window* wnd, lf::dim2<i32> extent) {
	glfwSetWindowSize(to_glfw_window(wnd), extent.width, extent.height);
}

lf::dim2<i32> get_platform_window_extent(const lf::platform_window* window) {
	lf::dim2<i32> result{};
	glfwGetWindowSize(const_cast<GLFWwindow*>(to_glfw_window(window)), &result.width, &result.height);
	return result;
}

bool platform_window_should_close(lf::platform_window* wnd) {
	return glfwWindowShouldClose(to_glfw_window(wnd));
}

void poll_events() {
	glfwPollEvents();
}

lf::result<VkSurface> create_platform_vulkan_surface(VkInstance instance, lf::platform_window* wnd) {
	VkSurfaceKHR created_surface = VK_NULL_HANDLE;
	VkResult result = glfwCreateWindowSurface(instance, to_glfw_window(wnd), nullptr, &created_surface);
	if (result != VK_SUCCESS) {
		return lf::unexpected(lf::generic_errc::unknown);
	}

	return created_surface;
}

namespace lf {
	PlatformAPI CreateGLFWPlatformAPI() {
		PlatformAPI api{};
		api.init = &platform_init;
		api.exit = &platform_exit;
		api.platform_vulkan_supported = &platform_vulkan_supported;
		api.get_platform_vulkan_instance_extensions = &get_platform_vulkan_instance_extensions;
		api.create_platform_window = &create_platform_window;
		api.destroy_platform_window = &destroy_platform_window;
		api.show_platform_window = &show_platform_window;
		api.hide_platform_window = &hide_platform_window;
		api.set_platform_window_extent = &set_platform_window_extent;
		api.get_platform_window_extent = &get_platform_window_extent;
		api.poll_events = &poll_events;
		api.create_platform_vulkan_surface = &create_platform_vulkan_surface;
		api.platform_window_should_close = &platform_window_should_close;
		return api;
	}
}
