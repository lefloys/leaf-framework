#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/string.hpp"
#include "resource.hpp"
#include "leaf/math/dim.hpp"

namespace lf {
	struct GraphicsAPI {
		error(*init)();
		void (*exit)();

		struct {
			handle<window>(*create)(string_view title, dim2<i32> extent);
			void (*destroy)(handle<window> wnd);
			void (*show)(view<window> wnd);
			void (*hide)(view<window> wnd);
			void (*resize)(view<window> wnd, dim2<i32> extent);
			dim2<i32>(*get_size)(view<const window> wnd);
			void (*acquire_image)(view<window> wnd);
			view<framebuffer>(*get_framebuffer)(view<window> wnd);
			void (*present)(view<window> wnd);
			bool (*should_close)(view<const window> wnd);
		} Window;
		struct {
			handle<command_buffer>(*create)();
			void (*destroy)(handle<command_buffer> cmd);
			void (*reset)(view<command_buffer> cmd_buf);
			void (*begin)(view<command_buffer> cmd_buf);
			void (*end)(view<command_buffer> cmd_buf);
			void (*draw)(view<command_buffer> cmd_buf, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
		} CommandBuffer;
		struct {
			void (*destroy)(handle<framebuffer> fb);
		} Framebuffer;
	};
	extern GraphicsAPI Graphics;
	void SetGraphicsAPI(GraphicsAPI api);
}
