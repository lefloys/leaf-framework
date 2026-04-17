#pragma once

#include "leaf/core/types.hpp"
#include "leaf/math/dim.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/error.hpp"

namespace lf::detail {
	struct platform_window;

	error platform_init();
	void platform_exit();

	platform_window* create_platform_window(string_view title, dim2<u32> extent);
	void destroy_platform_window(platform_window* window);

	void set_platform_window_title(platform_window* window, string_view title);
	string_view get_platform_window_title(platform_window* window);
	void set_platform_window_extent(platform_window* window, dim2<u32> extent);
	dim2<u32> get_platform_window_extent(platform_window* window);
}