#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/string.hpp"
#include "resource.hpp"
#include "leaf/math/dim.hpp"

namespace lf {
	struct GraphicsAPI {
		error (*init)();
		void (*exit)();

		struct {
			handle<window> (*create)(string_view title, dim2<i32> extent);
			void (*destroy)(handle<window> wnd);
			void (*show)(view<window> wnd);
			void (*hide)(view<window> wnd);
			void (*resize)(view<window> wnd, dim2<i32> extent);
			dim2<i32> (*get_size)(view<const window> wnd);
			view<framebuffer> (*begin_frame)(view<window> wnd);
			void (*end_frame)(view<window> wnd);
		} Window;
	};
	extern GraphicsAPI Graphics;
	void SetGraphicsAPI(GraphicsAPI api);
}
