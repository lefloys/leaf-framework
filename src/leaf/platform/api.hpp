#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include "leaf/math/dim.hpp"

struct VkInstance_T;
struct VkSurfaceKHR_T;

namespace lf {
	struct platform_window;
	using VkInstance = VkInstance_T*;
	using VkSurface = VkSurfaceKHR_T*;

	struct PlatformAPI {
		error (*init)();
		void (*exit)();

		bool (*platform_vulkan_supported)();
		const char** (*get_platform_vulkan_instance_extensions)(u32& extension_count);

		platform_window* (*create_platform_window)(string_view title, dim2<i32> extent);
		void (*destroy_platform_window)(platform_window* window);
		void (*show_platform_window)(platform_window* window);
		void (*hide_platform_window)(platform_window* window);
		void (*set_platform_window_extent)(platform_window* window, dim2<i32> extent);
		dim2<i32> (*get_platform_window_extent)(const platform_window* window);

		result<VkSurface> (*create_platform_vulkan_surface)(VkInstance instance, platform_window* window);
	};

	extern PlatformAPI Platform;

	void SetPlatformAPI(PlatformAPI api);
}
