#ifndef LEAF_WINDOW_BACKEND_HPP
#define LEAF_WINDOW_BACKEND_HPP

#include <leaf/graphics/window.hpp>

#include <rt_ext_swapchain.h>

namespace lf {

	struct PlatformWindow;

	struct PlatformWindowCreateInfo {
		string_view title;
		u32 width = 1280;
		u32 height = 720;
	};

	PlatformWindow* create_platform_window(const PlatformWindowCreateInfo& info);
	void destroy_platform_window(PlatformWindow* window);
	void bind_platform_window_swapchain(PlatformWindow* window, rt_swapchain swapchain);
	void platform_window_title(PlatformWindow* window, string_view title);
	void platform_window_size(PlatformWindow* window, dim2<u32> size);
	dim2<u32> platform_window_size(const PlatformWindow* window);
	bool platform_window_should_close(const PlatformWindow* window);
	void platform_window_should_close(PlatformWindow* window, bool should_close);
	bool platform_window_key_down(const PlatformWindow* window, Key key);
	bool platform_window_key_pressed(const PlatformWindow* window, Key key);
	bool platform_window_key_released(const PlatformWindow* window, Key key);
	bool platform_window_mouse_down(const PlatformWindow* window, MouseButton button);
	bool platform_window_mouse_pressed(const PlatformWindow* window, MouseButton button);
	bool platform_window_mouse_released(const PlatformWindow* window, MouseButton button);
	pos2<f32> platform_window_mouse_position(const PlatformWindow* window);
	f32 platform_window_scroll(const PlatformWindow* window);
	f32 platform_window_consume_scroll(PlatformWindow* window);
	dim2<u32> platform_window_framebuffer_size(const PlatformWindow* window);
	bool update_platform();

} // namespace lf

#endif /* LEAF_WINDOW_BACKEND_HPP */
