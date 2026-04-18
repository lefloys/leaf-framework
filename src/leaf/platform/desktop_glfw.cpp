#include "window.hpp"

#include "leaf/core/exception.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace lf::detail {
	namespace {
		platform_window* to_platform_window(GLFWwindow* window) {
			return reinterpret_cast<platform_window*>(window);
		}

		GLFWwindow* to_glfw_window(platform_window* window) {
			return reinterpret_cast<GLFWwindow*>(window);
		}

		const GLFWwindow* to_glfw_window(const platform_window* window) {
			return reinterpret_cast<const GLFWwindow*>(window);
		}
	}

	error platform_init() {
		if (!glfwInit()) {
			return error(generic_errc::unknown, "failed to initialize GLFW");
		}
		return error::no_error;
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

	platform_window* create_platform_window(string_view title, dim2<i32> extent) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

		GLFWwindow* glfw_window = glfwCreateWindow(static_cast<int>(extent.width), static_cast<int>(extent.height), title.data(), nullptr, nullptr);
		if (!glfw_window) {
			const char* description = nullptr;
			glfwGetError(&description);
			throw runtime_exception(description ? description : "failed to create GLFW window");
		}

		return to_platform_window(glfw_window);
	}

	void destroy_platform_window(platform_window* window) { glfwDestroyWindow(to_glfw_window(window)); }

	void show_platform_window(platform_window* window) { glfwShowWindow(to_glfw_window(window)); }
	void hide_platform_window(platform_window* window) { glfwHideWindow(to_glfw_window(window)); }
	void set_platform_window_extent(platform_window* window, dim2<i32> extent) { glfwSetWindowSize(to_glfw_window(window), static_cast<int>(extent.width), static_cast<int>(extent.height)); }

	dim2<i32> get_platform_window_extent(const platform_window* window) {
		int width = 0;
		int height = 0;
		glfwGetWindowSize(const_cast<GLFWwindow*>(to_glfw_window(window)), &width, &height);
		return { width, height };
	}

	result<vk_surface_handle> create_platform_vulkan_surface(vk_instance_handle instance, platform_window* window) {
		VkSurfaceKHR created_surface = VK_NULL_HANDLE;
		VkResult result = glfwCreateWindowSurface(reinterpret_cast<VkInstance>(instance), to_glfw_window(window), nullptr, &created_surface );
		if (result != VK_SUCCESS) {
			return unexpected(make_error_code(generic_errc::unknown));
		}

		return reinterpret_cast<vk_surface_handle>(created_surface);
	}
}
