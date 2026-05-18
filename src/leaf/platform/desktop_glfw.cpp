#include "platform.hpp"

#include <leaf/core/exception.hpp>

#include <GLFW/glfw3.h>
#include <rt_ext_glfw.h>
#include <rt_ext_swapchain.h>

#include <algorithm>
#include <array>
#include <vector>

static lf::PlatformWindow* from_glfw(GLFWwindow* wnd) { return reinterpret_cast<lf::PlatformWindow*>(wnd); }
static GLFWwindow* to_glfw(lf::PlatformWindow* wnd) { return reinterpret_cast<GLFWwindow*>(wnd); }

namespace lf {
	error init_platform(span<string_view> args) {
		if (!glfwInit()) {
			return error::unknown_error;
		}
		return error::no_error;
	}
	void exit_platform() {
		glfwTerminate();
	}

	PlatformWindow* create_platform_window(const PlatformWindowCreateInfo& info) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

		GLFWwindow* wnd = glfwCreateWindow(static_cast<i32>(info.width), static_cast<i32>(info.height), info.title.data(), nullptr, nullptr);
		if (!wnd) {
			const char* description = nullptr;
			glfwGetError(&description);
			if (description && description[0]) {
				throw runtime_exception(format("failed to create GLFW wnd: {}", description));
			}
			throw runtime_exception("failed to create GLFW wnd");
		}

		return from_glfw(wnd);
	}

	void destroy_platform_window(PlatformWindow* wnd) {
		glfwDestroyWindow(to_glfw(wnd));
	}

	void bind_platform_window_swapchain(PlatformWindow* wnd, rt_swapchain swapchain) {
		rtSwapchainBindWindowGLFW(swapchain, to_glfw(wnd));
		detail::check_rutile_error("failed to bind GLFW wnd to swapchain");
	}

	void platform_window_title(PlatformWindow* wnd, string_view title) {
		glfwSetWindowTitle(to_glfw(wnd), title.data());
	}

	void platform_window_size(PlatformWindow* wnd, dim2<u32> size) {
		glfwSetWindowSize(to_glfw(wnd), static_cast<i32>(size.width), static_cast<i32>(size.height));
	}

	dim2<u32> platform_window_size(PlatformWindow* wnd) {
		int width = 0;
		int height = 0;
		glfwGetWindowSize(to_glfw(wnd), &width, &height);
		return { static_cast<u32>(width), static_cast<u32>(height) };
	}

	bool platform_window_should_close(PlatformWindow* wnd) {
		return glfwWindowShouldClose(to_glfw(wnd));
	}

	void platform_window_should_close(PlatformWindow* wnd, bool should_close) {
		glfwSetWindowShouldClose(to_glfw(wnd), should_close ? GLFW_TRUE : GLFW_FALSE);
	}


	dim2<u32> platform_window_framebuffer_size(PlatformWindow* wnd) {
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(to_glfw(wnd), &width, &height);
		return { static_cast<u32>(width), static_cast<u32>(height) };
	}

	bool update_platform() {
		glfwWaitEvents();
		return true;
	}
} // namespace lf
