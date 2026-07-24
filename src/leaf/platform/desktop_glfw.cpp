#include "platform.hpp"

#include "leaf/core/exception.hpp"
#include "leaf/graphics/resource.hpp"
#include "leaf/graphics/window_private.hpp"
#include "leaf/core/logging.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <rt_ext_glfw.h>
#include <rt_ext_swapchain.h>

#include <algorithm>
#include <array>
#include <vector>

static rt::PlatformWindow* from_glfw(GLFWwindow* wnd) { return reinterpret_cast<rt::PlatformWindow*>(wnd); }
static GLFWwindow* to_glfw(rt::PlatformWindow* wnd) { return reinterpret_cast<GLFWwindow*>(wnd); }
static rt::window_t* owner(GLFWwindow* wnd) { return static_cast<rt::window_t*>(glfwGetWindowUserPointer(wnd)); }

static rt::input_key input_key_from_glfw(int key) {
	if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) { return static_cast<rt::input_key>(rt::KEY_A + key - GLFW_KEY_A); }
	if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) { return static_cast<rt::input_key>(rt::KEY_0 + key - GLFW_KEY_0); }
	if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) { return static_cast<rt::input_key>(rt::KEY_NUMPAD_0 + key - GLFW_KEY_KP_0); }
	if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F24) { return static_cast<rt::input_key>(rt::KEY_F1 + key - GLFW_KEY_F1); }

	switch (key) {
	case GLFW_KEY_BACKSPACE: /******/ return rt::KEY_BACKSPACE;
	case GLFW_KEY_TAB: /************/ return rt::KEY_TAB;
	case GLFW_KEY_ENTER: /**********/ return rt::KEY_ENTER;
	case GLFW_KEY_ESCAPE: /*********/ return rt::KEY_ESCAPE;
	case GLFW_KEY_SPACE: /**********/ return rt::KEY_SPACE;
	case GLFW_KEY_DELETE: /*********/ return rt::KEY_DELETE;
	case GLFW_KEY_INSERT: /*********/ return rt::KEY_INSERT;
	case GLFW_KEY_HOME: /***********/ return rt::KEY_HOME;
	case GLFW_KEY_END: /************/ return rt::KEY_END;
	case GLFW_KEY_PAGE_UP: /********/ return rt::KEY_PAGE_UP;
	case GLFW_KEY_PAGE_DOWN: /******/ return rt::KEY_PAGE_DOWN;
	case GLFW_KEY_LEFT: /***********/ return rt::KEY_LEFT_ARROW;
	case GLFW_KEY_RIGHT: /**********/ return rt::KEY_RIGHT_ARROW;
	case GLFW_KEY_UP: /*************/ return rt::KEY_UP_ARROW;
	case GLFW_KEY_DOWN: /***********/ return rt::KEY_DOWN_ARROW;
	case GLFW_KEY_LEFT_ALT: /*******/ return rt::KEY_ALT_LEFT;
	case GLFW_KEY_RIGHT_ALT: /******/ return rt::KEY_ALT_RIGHT;
	case GLFW_KEY_LEFT_CONTROL: /***/ return rt::KEY_CTRL_LEFT;
	case GLFW_KEY_RIGHT_CONTROL: /**/ return rt::KEY_CTRL_RIGHT;
	case GLFW_KEY_LEFT_SHIFT: /*****/ return rt::KEY_SHIFT_LEFT;
	case GLFW_KEY_RIGHT_SHIFT: /****/ return rt::KEY_SHIFT_RIGHT;
	case GLFW_KEY_LEFT_SUPER: /*****/ return rt::KEY_SUPER_LEFT;
	case GLFW_KEY_RIGHT_SUPER: /****/ return rt::KEY_SUPER_RIGHT;
	case GLFW_KEY_NUM_LOCK: /*******/ return rt::KEY_NUM_LOCK;
	case GLFW_KEY_SCROLL_LOCK: /****/ return rt::KEY_SCROLL_LOCK;
	case GLFW_KEY_CAPS_LOCK: /******/ return rt::KEY_CAPS_LOCK;
	case GLFW_KEY_PAUSE: /**********/ return rt::KEY_PAUSE;
	case GLFW_KEY_PRINT_SCREEN: /***/ return rt::KEY_PRINT;
	case GLFW_KEY_KP_ADD: /*********/ return rt::KEY_NUMPAD_ADD;
	case GLFW_KEY_KP_DECIMAL: /*****/ return rt::KEY_NUMPAD_DECIMAL;
	case GLFW_KEY_KP_DIVIDE: /******/ return rt::KEY_NUMPAD_DIVIDE;
	case GLFW_KEY_KP_ENTER: /*******/ return rt::KEY_NUMPAD_ENTER;
	case GLFW_KEY_KP_MULTIPLY: /****/ return rt::KEY_NUMPAD_MULTIPLY;
	case GLFW_KEY_KP_SUBTRACT: /****/ return rt::KEY_NUMPAD_SUBTRACT;
	case GLFW_KEY_GRAVE_ACCENT: /***/ return rt::KEY_BACKQUOTE;
	case GLFW_KEY_BACKSLASH: /******/ return rt::KEY_BACKSLASH;
	case GLFW_KEY_LEFT_BRACKET: /***/ return rt::KEY_BRACKET_LEFT;
	case GLFW_KEY_RIGHT_BRACKET: /**/ return rt::KEY_BRACKET_RIGHT;
	case GLFW_KEY_COMMA: /**********/ return rt::KEY_COMMA;
	case GLFW_KEY_EQUAL: /**********/ return rt::KEY_EQUAL;
	case GLFW_KEY_MINUS: /**********/ return rt::KEY_MINUS;
	case GLFW_KEY_PERIOD: /*********/ return rt::KEY_PERIOD;
	case GLFW_KEY_APOSTROPHE: /*****/ return rt::KEY_QUOTE;
	case GLFW_KEY_SEMICOLON: /******/ return rt::KEY_SEMICOLON;
	case GLFW_KEY_SLASH: /**********/ return rt::KEY_SLASH;
	case GLFW_KEY_MENU: /***********/ return rt::KEY_CONTEXT_MENU;
	default: return rt::KEY_NULL;
	}
}

static rt::input_modifiers input_modifiers_from_glfw(int mods) {
	rt::input_modifiers modifiers;
	if (mods & GLFW_MOD_CONTROL) { modifiers.add(rt::INPUT_MODIFIER_CTRL); }
	if (mods & GLFW_MOD_SHIFT) { modifiers.add(rt::INPUT_MODIFIER_SHIFT); }
	if (mods & GLFW_MOD_ALT) { modifiers.add(rt::INPUT_MODIFIER_ALT); }
	if (mods & GLFW_MOD_SUPER) { modifiers.add(rt::INPUT_MODIFIER_SUPER); }
	return modifiers;
}

static void mouse_button_callback(GLFWwindow* wnd, int button, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_RELEASE) {
		rt::window_t* window = owner(wnd);
		if (!window) {
			return;
		}
		rt::input_button input_button = static_cast<rt::input_button>(button + 1);
		bool down = action == GLFW_PRESS;
		double x = 0.0;
		double y = 0.0;
		glfwGetCursorPos(wnd, &x, &y);
		window->pointer({ static_cast<f32>(x), static_cast<f32>(y) });
		window->control(
			{ rt::INPUT_CONTROL_BUTTON, static_cast<u16>(input_button) },
			down,
			input_modifiers_from_glfw(mods));
	}
}

static void key_callback(GLFWwindow* wnd, int key, int, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_RELEASE || action == GLFW_REPEAT) {
		rt::input_key input_key = input_key_from_glfw(key);
		if (input_key != rt::KEY_NULL) {
			rt::window_t* window = owner(wnd);
			if (!window) {
				return;
			}
			bool down = action != GLFW_RELEASE;
			rt::input_modifiers modifiers = input_modifiers_from_glfw(mods);
			window->control({ rt::INPUT_CONTROL_KEY, static_cast<u16>(input_key) }, down, modifiers);
		}
	}
}

static void char_callback(GLFWwindow* wnd, unsigned int codepoint) {
	rt::window_t* window = owner(wnd);
	if (!window) {
		return;
	}
	window->text(static_cast<u32>(codepoint));
}

static void cursor_position_callback(GLFWwindow* wnd, double x, double y) {
	rt::window_t* window = owner(wnd);
	if (!window) {
		return;
	}
	window->pointer({
		static_cast<f32>(x),
		static_cast<f32>(y),
	});
}

static void cursor_enter_callback(GLFWwindow* wnd, int entered) {
	if (rt::window_t* window = owner(wnd)) {
		window->pointer_enter(entered == GLFW_TRUE);
	}
}

static void scroll_callback(GLFWwindow* wnd, double x, double y) {
	if (rt::window_t* window = owner(wnd)) {
		window->scroll({
			static_cast<f32>(x),
			static_cast<f32>(y),
		});
	}
}

static void focus_callback(GLFWwindow* wnd, int focused) {
	if (rt::window_t* window = owner(wnd)) {
		window->focus(focused == GLFW_TRUE);
	}
}

static void drop_callback(GLFWwindow* wnd, int count, const char** paths) {
	rt::window_t* window = owner(wnd);
	if (!window) {
		return;
	}
	for (int i = 0; i < count; ++i) {
		window->drop(paths[i]);
	}
}

static void framebuffer_size_callback(GLFWwindow* wnd, int width, int height) {
	if (width == 0 || height == 0) {
		return;
	}
	int window_width = 0;
	int window_height = 0;
	glfwGetWindowSize(wnd, &window_width, &window_height);
	if (window_width == 0 || window_height == 0) {
		return;
	}
	rt::window_t* window = owner(wnd);
	if (!window) {
		return;
	}
	window->resize(
		{
			static_cast<u32>(window_width),
			static_cast<u32>(window_height),
		},
		{
			static_cast<u32>(width),
			static_cast<u32>(height),
		});
}

namespace rt {
	error init_platform(span<string_view> args) {
		log::Info("[leaf] Starting platform...");
		if (!glfwInit()) {
			// TODO
			// to_error whatever();
			return error::unknown_error;
		}
		return error::no_error;
	}
	void exit_platform() {
		glfwTerminate();
	}
	string_view platform_backend_name() {
		return "GLFW";
	}

	PlatformWindow* create_platform_window(const PlatformWindowCreateInfo& info) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

		GLFWwindow* wnd = glfwCreateWindow(static_cast<i32>(info.width), static_cast<i32>(info.height), info.title.data(), nullptr, nullptr);
		if (!wnd) {
			// TODO
			// throw_glfw_error();
			throw lf::runtime_exception("failed to create GLFW window");
		}

		return from_glfw(wnd);
	}

	void destroy_platform_window(PlatformWindow* wnd) {
		glfwSetMouseButtonCallback(to_glfw(wnd), nullptr);
		glfwSetKeyCallback(to_glfw(wnd), nullptr);
		glfwSetCharCallback(to_glfw(wnd), nullptr);
		glfwSetCursorPosCallback(to_glfw(wnd), nullptr);
		glfwSetCursorEnterCallback(to_glfw(wnd), nullptr);
		glfwSetScrollCallback(to_glfw(wnd), nullptr);
		glfwSetWindowFocusCallback(to_glfw(wnd), nullptr);
		glfwSetDropCallback(to_glfw(wnd), nullptr);
		glfwSetFramebufferSizeCallback(to_glfw(wnd), nullptr);
		glfwSetWindowUserPointer(to_glfw(wnd), nullptr);
		glfwDestroyWindow(to_glfw(wnd));
	}

	void bind_platform_window_swapchain(PlatformWindow* wnd, rt_swapchain swapchain) {
		rtSwapchainBindWindowGLFW(swapchain, to_glfw(wnd));
		detail::check_rutile_error("failed to bind GLFW wnd to swapchain");
	}

	void platform_window_owner(PlatformWindow* wnd, window_t* owner) {
		if (!wnd) {
			return;
		}
		glfwSetWindowUserPointer(to_glfw(wnd), owner);
		glfwSetMouseButtonCallback(to_glfw(wnd), mouse_button_callback);
		glfwSetKeyCallback(to_glfw(wnd), key_callback);
		glfwSetCharCallback(to_glfw(wnd), char_callback);
		glfwSetCursorPosCallback(to_glfw(wnd), cursor_position_callback);
		glfwSetCursorEnterCallback(to_glfw(wnd), cursor_enter_callback);
		glfwSetScrollCallback(to_glfw(wnd), scroll_callback);
		glfwSetWindowFocusCallback(to_glfw(wnd), focus_callback);
		glfwSetDropCallback(to_glfw(wnd), drop_callback);
		glfwSetFramebufferSizeCallback(to_glfw(wnd), framebuffer_size_callback);
	}

	void platform_window_clear_owner(PlatformWindow* wnd) {
		if (!wnd) {
			return;
		}
		glfwSetWindowUserPointer(to_glfw(wnd), nullptr);
	}

	void platform_window_title(PlatformWindow* wnd, string_view title) {
		glfwSetWindowTitle(to_glfw(wnd), title.data());
	}

	void platform_window_show(PlatformWindow* wnd) {
		glfwShowWindow(to_glfw(wnd));
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

	dim2<u32> platform_framebuffer_size(PlatformWindow* wnd) {
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(to_glfw(wnd), &width, &height);
		return { static_cast<u32>(width), static_cast<u32>(height) };
	}

	bool platform_window_drawable(PlatformWindow* wnd) {
		GLFWwindow* glfw_window = to_glfw(wnd);
		if (glfwGetWindowAttrib(glfw_window, GLFW_ICONIFIED) == GLFW_TRUE) {
			return false;
		}

		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(glfw_window, &width, &height);
		return width > 0 && height > 0;
	}

	void platform_window_position(PlatformWindow* wnd, pos2<i32> position) {
		glfwSetWindowPos(to_glfw(wnd), position.x, position.y);
	}

	pos2<i32> platform_window_position(PlatformWindow* wnd) {
		int x = 0;
		int y = 0;
		glfwGetWindowPos(to_glfw(wnd), &x, &y);
		return { static_cast<i32>(x), static_cast<i32>(y) };
	}

	void platform_window_fullscreen(PlatformWindow* wnd, bool fullscreen, pos2<i32> windowed_position, dim2<u32> windowed_size) {
		GLFWwindow* window = to_glfw(wnd);
		if (fullscreen) {
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			if (!monitor) {
				return;
			}
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			if (!mode) {
				return;
			}
			int monitor_x = 0;
			int monitor_y = 0;
			glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);
			glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
			glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);
			glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_FALSE);
			glfwSetWindowMonitor(window, nullptr, monitor_x, monitor_y, mode->width, mode->height, GLFW_DONT_CARE);
			glfwFocusWindow(window);
			return;
		}

		glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_FALSE);
		glfwSetWindowMonitor(
			window,
			nullptr,
			windowed_position.x,
			windowed_position.y,
			static_cast<int>(windowed_size.width),
			static_cast<int>(windowed_size.height),
			GLFW_DONT_CARE);
		glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
		glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_TRUE);
	}

	bool platform_window_should_close(PlatformWindow* wnd) {
		return glfwWindowShouldClose(to_glfw(wnd));
	}

	void platform_window_should_close(PlatformWindow* wnd, bool should_close) {
		glfwSetWindowShouldClose(to_glfw(wnd), should_close ? GLFW_TRUE : GLFW_FALSE);
	}

	PlatformCursor* create_platform_cursor(const u08* rgba, u32 width, u32 height, u32 hotspot_x, u32 hotspot_y) {
		GLFWimage image;
		image.width = static_cast<int>(width);
		image.height = static_cast<int>(height);
		image.pixels = const_cast<unsigned char*>(rgba);
		return reinterpret_cast<PlatformCursor*>(glfwCreateCursor(&image, static_cast<int>(hotspot_x), static_cast<int>(hotspot_y)));
	}

	void destroy_platform_cursor(PlatformCursor* cursor) {
		if (cursor) {
			glfwDestroyCursor(reinterpret_cast<GLFWcursor*>(cursor));
		}
	}

	void platform_window_cursor(PlatformWindow* wnd, PlatformCursor* cursor) {
		glfwSetCursor(to_glfw(wnd), reinterpret_cast<GLFWcursor*>(cursor));
	}

	bool update_platform() {
		glfwWaitEventsTimeout(1.0 / 120.0);
		return true;
	}
	void platform_clipboard_text(string_view text) {
		glfwSetClipboardString(nullptr, text.data());
	}

	string platform_clipboard_text() {
		const char* text = glfwGetClipboardString(nullptr);
		return text ? string(text) : string();
	}
} // namespace rt
