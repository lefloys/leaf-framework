#ifndef LEAF_GRAPHICS_WINDOW_HPP
#define LEAF_GRAPHICS_WINDOW_HPP

#include <leaf/graphics/resource.hpp>
#include <leaf/math/dim.hpp>
#include <leaf/math/pos.hpp>

namespace lf {

	struct window_t;

	template <>
	struct resource_traits<window> {
		using native_handle = window_t*;
		static void destroy(native_handle handle);
	};

	enum class Key {
		Escape,
		W,
		A,
		S,
		D,
		Left,
		Right,
		Up,
		Down,
	};

	enum class MouseButton {
		Left,
		Right,
		Middle,
	};

	namespace Window {
		handle<window> Create();
		void Destroy(handle<window> window);
		void SetTitle(view<window> window, string_view title);
		void SetWidth(view<window> window, u32 width);
		void SetHeight(view<window> window, u32 height);
		bool ShouldClose(view<const window> window);
		void SetShouldClose(view<window> window, bool should_close);

		dim2<u32> FramebufferSize(view<const window> window);
		handle<framebuffer> CurrentFramebuffer(view<const window> window);
		view<command_buffer> BeginFrame(view<window> window, view<queue> queue);
		void EndFrame(view<window> window);
	} // namespace Window

} // namespace lf

#endif /* LEAF_GRAPHICS_WINDOW_HPP */
