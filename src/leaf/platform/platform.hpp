#pragma once

#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"
#include "leaf/graphics/window.hpp"

#include <rt_ext_swapchain.h>




namespace lf {
	struct PlatformWindow;

	struct PlatformWindowCreateInfo {
		string_view title;
		u32 width = 1280;
		u32 height = 720;
	};

	error init_platform(span<string_view> args);
	void exit_platform();

	PlatformWindow* create_platform_window(const PlatformWindowCreateInfo& info);
	void destroy_platform_window(PlatformWindow* window);
	void bind_platform_window_swapchain(PlatformWindow* window, rt_swapchain swapchain);
	void platform_window_title(PlatformWindow* window, string_view title);
	void platform_window_size(PlatformWindow* window, dim2<u32> size);
	dim2<u32> platform_window_size(PlatformWindow* window);
	bool platform_window_should_close(PlatformWindow* window);
	void platform_window_should_close(PlatformWindow* window, bool should_close);
	bool platform_window_key_down(PlatformWindow* window, Key key);
	bool platform_window_key_pressed(PlatformWindow* window, Key key);
	bool platform_window_key_released(PlatformWindow* window, Key key);
	bool platform_window_mouse_down(PlatformWindow* window, MouseButton button);
	bool platform_window_mouse_pressed(PlatformWindow* window, MouseButton button);
	bool platform_window_mouse_released(PlatformWindow* window, MouseButton button);
	pos2<f32> platform_window_mouse_position(PlatformWindow* window);
	f32 platform_window_scroll(PlatformWindow* window);
	f32 platform_window_consume_scroll(PlatformWindow* window);
	dim2<u32> platform_window_framebuffer_size(PlatformWindow* window);
	bool update_platform();

} // namespace lf
