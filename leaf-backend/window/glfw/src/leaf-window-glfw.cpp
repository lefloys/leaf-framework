#include "leaf-window-glfw.hpp"

#include <leaf/native_window.hpp>
#include <leaf/core/error.hpp>

#include <GLFW/glfw3.h>

#include <string>

#define LF_BACKEND_EXPORT extern "C" __declspec(dllexport)

namespace lf::glfw {
	error init_window_backend() {
		if (glfwInit() != GLFW_TRUE) {
			return error(generic_errc::unknown, "GLFW initialization failed");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		return error();
	}

	void exit_window_backend() {
		glfwTerminate();
	}

	NativeWindow* create_window(std::string_view title, dim2<u32> extent) {
		const std::string title_str(title);
		auto* native = glfwCreateWindow(static_cast<int>(extent.width), static_cast<int>(extent.height), title_str.c_str(), nullptr, nullptr);
		if (!native) {
			return nullptr;
		}

		return reinterpret_cast<NativeWindow*>(native);
	}

	void destroy_window(NativeWindow* native) {
		glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(native));
	}

	const WindowBackend* GetWindowBackend() {
		static WindowBackend backend = {
			.init = init_window_backend,
			.exit = exit_window_backend,
			.create_window = create_window,
			.destroy_window = destroy_window,
		};
		return &backend;
	}
}

LF_BACKEND_EXPORT const lf::WindowBackend* lf_get_window_backend() {
	return lf::glfw::GetWindowBackend();
}
