#include "../backend.hpp"

#include <leaf/core/exception.hpp>

#include <GLFW/glfw3.h>
#include <rt_ext_glfw.h>
#include <rt_ext_swapchain.h>

#include <algorithm>
#include <array>
#include <vector>

namespace lf {
	namespace detail {
		static constexpr usize key_count = 9;
		static constexpr usize mouse_button_count = 3;

		struct InputSnapshot {
			std::array<bool, key_count> current_keys{};
			std::array<bool, key_count> previous_keys{};
			std::array<bool, mouse_button_count> current_mouse{};
			std::array<bool, mouse_button_count> previous_mouse{};
		};
	} // namespace detail

	struct PlatformWindow {
		GLFWwindow* glfw = nullptr;
		detail::InputSnapshot input;
		f32 scroll_offset = 0.0f;
	};

	namespace detail {
		static std::vector<PlatformWindow*> windows;

		int to_glfw_key(Key key) {
			switch (key) {
			case Key::W: return GLFW_KEY_W;
			case Key::A: return GLFW_KEY_A;
			case Key::S: return GLFW_KEY_S;
			case Key::D: return GLFW_KEY_D;
			case Key::Left: return GLFW_KEY_LEFT;
			case Key::Right: return GLFW_KEY_RIGHT;
			case Key::Up: return GLFW_KEY_UP;
			case Key::Down: return GLFW_KEY_DOWN;
			case Key::Escape: return GLFW_KEY_ESCAPE;
			default: return GLFW_KEY_UNKNOWN;
			}
		}

		int to_glfw_mouse_button(MouseButton button) {
			switch (button) {
			case MouseButton::Left: return GLFW_MOUSE_BUTTON_LEFT;
			case MouseButton::Right: return GLFW_MOUSE_BUTTON_RIGHT;
			case MouseButton::Middle: return GLFW_MOUSE_BUTTON_MIDDLE;
			default: return GLFW_MOUSE_BUTTON_LEFT;
			}
		}

		usize key_index(Key key) {
			return static_cast<usize>(key);
		}

		usize mouse_button_index(MouseButton button) {
			return static_cast<usize>(button);
		}

		bool read_key(GLFWwindow* window, Key key) {
			i32 glfw_key = to_glfw_key(key);
			return window && glfw_key != GLFW_KEY_UNKNOWN && glfwGetKey(window, glfw_key) == GLFW_PRESS;
		}

		bool read_mouse(GLFWwindow* window, MouseButton button) {
			return window && glfwGetMouseButton(window, to_glfw_mouse_button(button)) == GLFW_PRESS;
		}

		void refresh_input_snapshot(PlatformWindow& window) {
			InputSnapshot& snapshot = window.input;
			snapshot.previous_keys = snapshot.current_keys;
			snapshot.previous_mouse = snapshot.current_mouse;
			snapshot.current_keys[key_index(Key::Escape)] = read_key(window.glfw, Key::Escape);
			snapshot.current_keys[key_index(Key::W)] = read_key(window.glfw, Key::W);
			snapshot.current_keys[key_index(Key::A)] = read_key(window.glfw, Key::A);
			snapshot.current_keys[key_index(Key::S)] = read_key(window.glfw, Key::S);
			snapshot.current_keys[key_index(Key::D)] = read_key(window.glfw, Key::D);
			snapshot.current_keys[key_index(Key::Left)] = read_key(window.glfw, Key::Left);
			snapshot.current_keys[key_index(Key::Right)] = read_key(window.glfw, Key::Right);
			snapshot.current_keys[key_index(Key::Up)] = read_key(window.glfw, Key::Up);
			snapshot.current_keys[key_index(Key::Down)] = read_key(window.glfw, Key::Down);
			snapshot.current_mouse[mouse_button_index(MouseButton::Left)] = read_mouse(window.glfw, MouseButton::Left);
			snapshot.current_mouse[mouse_button_index(MouseButton::Right)] = read_mouse(window.glfw, MouseButton::Right);
			snapshot.current_mouse[mouse_button_index(MouseButton::Middle)] = read_mouse(window.glfw, MouseButton::Middle);
		}

		void scroll_callback(GLFWwindow* glfw, double xoffset, double yoffset) {
			(void)xoffset;
			auto* window = static_cast<PlatformWindow*>(glfwGetWindowUserPointer(glfw));
			if (window) {
				window->scroll_offset += static_cast<f32>(yoffset);
			}
		}
	} // namespace detail

	PlatformWindow* create_platform_window(const PlatformWindowCreateInfo& info) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

		GLFWwindow* glfw = glfwCreateWindow(static_cast<i32>(info.width), static_cast<i32>(info.height), info.title.data(), nullptr, nullptr);
		if (!glfw) {
			const char* description = nullptr;
			glfwGetError(&description);
			if (description && description[0]) {
				throw runtime_exception(format("failed to create GLFW window: {}", description));
			}
			throw runtime_exception("failed to create GLFW window");
		}

		auto* window = new PlatformWindow;
		window->glfw = glfw;
		glfwSetWindowUserPointer(glfw, window);
		glfwSetScrollCallback(glfw, detail::scroll_callback);
		detail::refresh_input_snapshot(*window);
		detail::windows.push_back(window);
		return window;
	}

	void destroy_platform_window(PlatformWindow* window) {
		if (!window) {
			return;
		}
		if (window->glfw) {
			glfwSetWindowUserPointer(window->glfw, nullptr);
			glfwDestroyWindow(window->glfw);
		}
		auto it = std::find(detail::windows.begin(), detail::windows.end(), window);
		if (it != detail::windows.end()) {
			detail::windows.erase(it);
		}
		delete window;
	}

	void bind_platform_window_swapchain(PlatformWindow* window, rt_swapchain swapchain) {
		if (window && window->glfw) {
			rtSwapchainBindWindowGLFW(swapchain, window->glfw);
			detail::check_rutile_error("failed to bind GLFW window to swapchain");
		}
	}

	void platform_window_title(PlatformWindow* window, string_view title) {
		if (window && window->glfw) {
			glfwSetWindowTitle(window->glfw, title.data());
		}
	}

	void platform_window_size(PlatformWindow* window, dim2<u32> size) {
		if (window && window->glfw) {
			glfwSetWindowSize(window->glfw, static_cast<int>(size.width), static_cast<int>(size.height));
		}
	}

	dim2<u32> platform_window_size(const PlatformWindow* window) {
		if (!window || !window->glfw) {
			return {};
		}
		int width = 0;
		int height = 0;
		glfwGetWindowSize(window->glfw, &width, &height);
		if (width <= 0 || height <= 0) {
			return {};
		}
		return { static_cast<u32>(width), static_cast<u32>(height) };
	}

	bool platform_window_should_close(const PlatformWindow* window) {
		return window && window->glfw && glfwWindowShouldClose(window->glfw);
	}

	void platform_window_should_close(PlatformWindow* window, bool should_close) {
		if (window && window->glfw) {
			glfwSetWindowShouldClose(window->glfw, should_close ? GLFW_TRUE : GLFW_FALSE);
		}
	}

	bool platform_window_key_down(const PlatformWindow* window, Key key) {
		return window && window->input.current_keys[detail::key_index(key)];
	}

	bool platform_window_key_pressed(const PlatformWindow* window, Key key) {
		return window && window->input.current_keys[detail::key_index(key)] && !window->input.previous_keys[detail::key_index(key)];
	}

	bool platform_window_key_released(const PlatformWindow* window, Key key) {
		return window && !window->input.current_keys[detail::key_index(key)] && window->input.previous_keys[detail::key_index(key)];
	}

	bool platform_window_mouse_down(const PlatformWindow* window, MouseButton button) {
		return window && window->input.current_mouse[detail::mouse_button_index(button)];
	}

	bool platform_window_mouse_pressed(const PlatformWindow* window, MouseButton button) {
		return window && window->input.current_mouse[detail::mouse_button_index(button)] && !window->input.previous_mouse[detail::mouse_button_index(button)];
	}

	bool platform_window_mouse_released(const PlatformWindow* window, MouseButton button) {
		return window && !window->input.current_mouse[detail::mouse_button_index(button)] && window->input.previous_mouse[detail::mouse_button_index(button)];
	}

	pos2<f32> platform_window_mouse_position(const PlatformWindow* window) {
		if (!window || !window->glfw) {
			return {};
		}
		double x = 0.0;
		double y = 0.0;
		glfwGetCursorPos(window->glfw, &x, &y);
		return { static_cast<f32>(x), static_cast<f32>(y) };
	}

	f32 platform_window_scroll(const PlatformWindow* window) {
		if (!window) {
			return 0.0f;
		}
		return window->scroll_offset;
	}

	f32 platform_window_consume_scroll(PlatformWindow* window) {
		if (!window) {
			return 0.0f;
		}
		f32 scroll = window->scroll_offset;
		window->scroll_offset = 0.0f;
		return scroll;
	}

	dim2<u32> platform_window_framebuffer_size(const PlatformWindow* window) {
		if (!window || !window->glfw) {
			return {};
		}
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(window->glfw, &width, &height);
		if (width <= 0 || height <= 0) {
			return {};
		}
		return { static_cast<u32>(width), static_cast<u32>(height) };
	}

	bool update_platform() {
		glfwPollEvents();
		for (PlatformWindow* window : detail::windows) {
			if (window) {
				detail::refresh_input_snapshot(*window);
			}
		}
		return true;
	}
} // namespace lf
