#include "window.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "leaf/core/exception.hpp"

namespace lf::detail {

	static platform_window* to_platform_window(GLFWwindow* window) { return reinterpret_cast<platform_window*>(window); }
	static GLFWwindow* to_glfw_window(platform_window* window) { return reinterpret_cast<GLFWwindow*>(window); }

	error platform_init() {
		if (!glfwInit()) {
			return error::unknown_error;
		}
		return error::no_error;
	}
	platform_window* create_platform_window(string_view title, dim2<u32> extent) {
		GLFWwindow* window = glfwCreateWindow(extent.width, extent.height, title.data(), nullptr, nullptr);

		if (!window) {
			const char* description;
			glfwGetError(&description);
			throw lf::runtime_exception(description);
		}

		return to_platform_window(window);
	}
}