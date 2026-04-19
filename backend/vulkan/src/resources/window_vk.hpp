#pragma once

#include "../object_allocator.hpp"
#include "../vulkan_context.hpp"
#include "resource.hpp"

#include "leaf/core/string.hpp"
#include "leaf/math/dim.hpp"
#include "leaf/platform/api.hpp"

#include <vulkan/vulkan.h>

struct WindowVK : Resource {
	lf::platform_window* platform_window = nullptr;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	WindowVK(lf::string_view title, lf::dim2<i32> extent);
	~WindowVK();
};

namespace Window {
	lf::handle<lf::window> Create(lf::string_view title, lf::dim2<i32> extent);
	void Destroy(lf::handle<lf::window> wnd);
	void Show(lf::view<lf::window> wnd);
	void Hide(lf::view<lf::window> wnd);
	void Resize(lf::view<lf::window> wnd, lf::dim2<i32> extent);
	lf::dim2<i32> GetSize(lf::view<const lf::window> wnd);
	lf::view<lf::framebuffer> BeginFrame(lf::view<lf::window> wnd);
	void EndFrame(lf::view<lf::window> wnd);
}

template<>
struct lf::type_name_trait<WindowVK> {
	static constexpr lf::string_view value = "window";
};
