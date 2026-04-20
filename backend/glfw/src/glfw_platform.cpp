#include <leaf/platform/backends/glfw.hpp>

#include "leaf/core/cstddef.hpp"
#include "leaf/core/exception.hpp"
#include "leaf/core/vector.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace {
	lf::vector<GLFWwindow*> g_windows;
}

lf::platform_window* to_platform_window(GLFWwindow* window) {
	return reinterpret_cast<lf::platform_window*>(window);
}

GLFWwindow* to_glfw_window(lf::platform_window* window) {
	return reinterpret_cast<GLFWwindow*>(window);
}

const GLFWwindow* to_glfw_window(const lf::platform_window* window) {
	return reinterpret_cast<const GLFWwindow*>(window);
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

	g_windows.push_back(glfw_window);
	return to_platform_window(glfw_window);
}

void destroy_platform_window(lf::platform_window* window) {
	GLFWwindow* glfw_window = to_glfw_window(window);
	for (lf::size_t index = 0; index < g_windows.size(); ++index) {
		if (g_windows[index] == glfw_window) {
			g_windows.erase(g_windows.begin() + index);
			break;
		}
	}
	glfwDestroyWindow(glfw_window);
}

void show_platform_window(lf::platform_window* window) {
	glfwShowWindow(to_glfw_window(window));
}

void hide_platform_window(lf::platform_window* window) {
	glfwHideWindow(to_glfw_window(window));
}

void set_platform_window_extent(lf::platform_window* window, lf::dim2<i32> extent) {
	glfwSetWindowSize(to_glfw_window(window), static_cast<int>(extent.width), static_cast<int>(extent.height));
}

lf::dim2<i32> get_platform_window_extent(const lf::platform_window* window) {
	lf::dim2<i32> result{};
	glfwGetWindowSize(const_cast<GLFWwindow*>(to_glfw_window(window)), &result.width, &result.height);
	return result;
}

void poll_events() {
	glfwPollEvents();
}

bool any_window_should_close() {
	for (GLFWwindow* window : g_windows) {
		if (glfwWindowShouldClose(window)) {
			return true;
		}
	}
	return false;
}

lf::result<lf::VkSurface> create_platform_vulkan_surface(lf::VkInstance instance, lf::platform_window* window) {
	VkSurfaceKHR created_surface = VK_NULL_HANDLE;
	VkResult result = glfwCreateWindowSurface(instance, to_glfw_window(window), nullptr, &created_surface);
	if (result != VK_SUCCESS) {
		return lf::unexpected(lf::generic_errc::unknown);
	}

	return reinterpret_cast<lf::VkSurface>(created_surface);
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
		api.any_window_should_close = &any_window_should_close;
		api.create_platform_vulkan_surface = &create_platform_vulkan_surface;
		return api;
	}
}
