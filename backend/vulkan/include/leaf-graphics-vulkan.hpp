#pragma once

#include <leaf/graphics/backends/vulkan.hpp>

namespace lf::detail::vk {
	error Init();
	void Exit();

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
