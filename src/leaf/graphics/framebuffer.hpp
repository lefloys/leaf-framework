#ifndef LEAF_GRAPHICS_FRAMEBUFFER_HPP
#define LEAF_GRAPHICS_FRAMEBUFFER_HPP

#include <leaf/graphics/resource.hpp>

namespace lf {
	namespace Framebuffer {
		handle<framebuffer> Create();
		void Destroy(handle<framebuffer> framebuffer);
		view<texture_view> ColorView(view<framebuffer> framebuffer, u32 slot);
		void ColorView(view<framebuffer> framebuffer, u32 slot, view<texture_view> attachment);
		void DepthView(view<framebuffer> framebuffer, view<texture_view> attachment);
	} // namespace Framebuffer
} // namespace lf

#endif /* LEAF_GRAPHICS_FRAMEBUFFER_HPP */
