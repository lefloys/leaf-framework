#pragma once

#include "leaf/core/types.hpp"
#include "leaf/core/string.hpp"
#include "leaf/math/dim.hpp"
#include "detail/resource.hpp"

namespace lf::Window {
	handle<window> Create(string_view title, dim2<i32> extent);
	void Destroy(handle<window> wnd);
	void Show(view<window> wnd);
	void Hide(view<window> wnd);

	dim2<i32> GetSize(view<const window> wnd);
	void Resize(view<window> wnd, dim2<i32> extent);

	view<framebuffer> BeginFrame(view<window> wnd);
	void EndFrame(view<window> wnd);
}
