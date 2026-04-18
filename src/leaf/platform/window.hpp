#pragma once

#include "leaf/core/types.hpp"
#include "leaf/math/dim.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/error.hpp"

struct VkInstance_T;
struct VkSurfaceKHR_T;

namespace lf::detail {
	struct platform_window;
	using vk_instance_handle = VkInstance_T*;
	using vk_surface_handle = VkSurfaceKHR_T*;

	error platform_init();
	void platform_exit();

	bool platform_vulkan_supported();
	const char** get_platform_vulkan_instance_extensions(u32& extension_count);

	platform_window* create_platform_window(string_view title, dim2<i32> extent);
	void destroy_platform_window(platform_window* window);
	void show_platform_window(platform_window* window);
	void hide_platform_window(platform_window* window);
	void set_platform_window_extent(platform_window* window, dim2<i32> extent);
	dim2<i32> get_platform_window_extent(const platform_window* window);

	result<vk_surface_handle> create_platform_vulkan_surface(vk_instance_handle instance, platform_window* window);
}
