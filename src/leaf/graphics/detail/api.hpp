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
			void (*acquire_image)(view<window> wnd);
			view<framebuffer> (*get_framebuffer)(view<window> wnd);
			void (*present)(view<window> wnd);
		} Window;
		struct {
			handle<command_buffer> (*create)();
			void (*destroy)(handle<command_buffer> cmd);
			void (*reset)(view<command_buffer> cmd);
			void (*begin_recording)(view<command_buffer> cmd);
			void (*end_recording)(view<command_buffer> cmd);
			void (*draw)(view<command_buffer> cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
		} CommandBuffer;
		struct {
			void (*destroy)(handle<framebuffer> fb);
			void (*flush)(view<framebuffer> fb);
			void (*submit)(view<framebuffer> fb, view<const command_buffer> cmd);
		} Framebuffer;
	};
	extern GraphicsAPI Graphics;
	void SetGraphicsAPI(GraphicsAPI api);
}
