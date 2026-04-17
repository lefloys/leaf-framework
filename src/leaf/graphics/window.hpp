#pragma once

#include "leaf/core/types.hpp"
#include "leaf/math/dim.hpp"
#include "detail/resource.hpp"

#include <string>

namespace lf::Window {
	handle<window> Create(std::string_view title, dim2<u32> extent);
	void Destroy(handle<window> wnd);

	dim2<u32> GetSize(view<const window> wnd);
	void SetSize(view<window> wnd, dim2<u32> extent);

	view<framebuffer> BeginFrame();
	void EndFrame();
}
