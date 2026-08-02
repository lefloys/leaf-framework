#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include "leaf/graphics/window.hpp"
#include "leaf/math/dim.hpp"
#include "leaf/math/pos.hpp"

#include <rt_ext_swapchain.h>

namespace rt {
	struct PlatformWindow;
	struct PlatformCursor;
	struct window_t;
	using lf::dim2;
	using lf::pos2;

	struct PlatformWindowCreateInfo {
		string_view title;
		u32 width = 1280;
		u32 height = 720;
	};

	error init_platform(span<string_view> args);
	void exit_platform();
	string_view platform_backend_name();

	PlatformWindow* create_platform_window(const PlatformWindowCreateInfo& info);
	void destroy_platform_window(PlatformWindow* window);
	void bind_platform_window_swapchain(PlatformWindow* window, rt_swapchain swapchain);
	void platform_window_owner(PlatformWindow* window, window_t* owner);
	void platform_window_clear_owner(PlatformWindow* window);
	void platform_window_title(PlatformWindow* window, string_view title);
	void platform_window_show(PlatformWindow* window);
	void platform_window_size(PlatformWindow* window, dim2<u32> size);
	dim2<u32> platform_window_size(PlatformWindow* window);
	dim2<u32> platform_framebuffer_size(PlatformWindow* window);
	bool platform_window_drawable(PlatformWindow* window);
	void platform_window_position(PlatformWindow* window, pos2<i32> position);
	pos2<i32> platform_window_position(PlatformWindow* window);
	void platform_window_fullscreen(PlatformWindow* window, bool fullscreen, pos2<i32> windowed_position, dim2<u32> windowed_size);
	bool platform_window_should_close(PlatformWindow* window);
	void platform_window_should_close(PlatformWindow* window, bool should_close);
	PlatformCursor* create_platform_cursor(const u08* rgba, u32 width, u32 height, u32 hotspot_x, u32 hotspot_y);
	void destroy_platform_cursor(PlatformCursor* cursor);
	void platform_window_cursor(PlatformWindow* window, PlatformCursor* cursor);
	void platform_clipboard_text(string_view text);
	string platform_clipboard_text();
	bool update_platform();

} // namespace rt

namespace lf {
	struct PlatformCursor;

	using rt::bind_platform_window_swapchain;
	using rt::create_platform_window;
	using rt::destroy_platform_window;
	using rt::exit_platform;
	using rt::init_platform;
	using rt::input_key;
	using rt::platform_backend_name;
	using rt::platform_clipboard_text;
	using rt::platform_framebuffer_size;
	using rt::platform_window_clear_owner;
	using rt::platform_window_drawable;
	using rt::platform_window_fullscreen;
	using rt::platform_window_owner;
	using rt::platform_window_position;
	using rt::platform_window_should_close;
	using rt::platform_window_show;
	using rt::platform_window_size;
	using rt::platform_window_title;
	using rt::PlatformWindow;
	using rt::PlatformWindowCreateInfo;
	using rt::update_platform;

	inline PlatformCursor* create_platform_cursor(const u08* rgba, u32 width, u32 height, u32 hotspot_x, u32 hotspot_y) {
		return reinterpret_cast<PlatformCursor*>(rt::create_platform_cursor(rgba, width, height, hotspot_x, hotspot_y));
	}

	inline void destroy_platform_cursor(PlatformCursor* cursor) {
		rt::destroy_platform_cursor(reinterpret_cast<rt::PlatformCursor*>(cursor));
	}

	inline void platform_window_cursor(PlatformWindow* window, PlatformCursor* cursor) {
		rt::platform_window_cursor(window, reinterpret_cast<rt::PlatformCursor*>(cursor));
	}
} // namespace lf
