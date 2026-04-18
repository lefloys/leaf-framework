#pragma once

#include "../object_allocator.hpp"
#include "../vulkan_context.hpp"
#include "resource.hpp"

#include "leaf/core/string.hpp"
#include "leaf/platform/window.hpp"

#include <vulkan/vulkan.h>

namespace lf::detail::vk {
	struct WindowVK : Resource {
		platform_window* platform_window = nullptr;
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		WindowVK(string_view title, dim2<i32> extent);
		~WindowVK();
	};

	template<>
	resource_pool<resource::window, WindowVK>& Pool<resource::window, WindowVK>();

	namespace Window {
		handle<window> Create(string_view title, dim2<i32> extent);
		void Destroy(handle<window> wnd);
		void Show(view<window> wnd);
		void Hide(view<window> wnd);
		void Resize(view<window> wnd, dim2<i32> extent);
		dim2<i32> GetSize(view<const window> wnd);
		view<framebuffer> BeginFrame(view<window> wnd);
		void EndFrame(view<window> wnd);
	}
}

template<>
struct lf::type_name_trait<lf::detail::vk::WindowVK> {
	static constexpr lf::string_view value = "window";
};
