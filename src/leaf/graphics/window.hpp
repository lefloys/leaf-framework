#pragma once

#include "detail/resource.hpp"
#include "leaf/core/types.hpp"
#include "leaf/math/dim.hpp"

#include <string>

namespace lf {
	struct Window : detail::Resource<Window> {
		static handle<Window> Create(std::string_view title, u32 width, u32 height);

		static dim2<u32> GetSize(view<const Window> wnd);
		static void SetSize(view<Window> wnd, dim2<u32> extent);


	};
}
