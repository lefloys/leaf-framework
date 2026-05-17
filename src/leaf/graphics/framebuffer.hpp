#ifndef LEAF_GRAPHICS_FRAMEBUFFER_HPP
#define LEAF_GRAPHICS_FRAMEBUFFER_HPP

#include <leaf/graphics/resource.hpp>

namespace lf {
	namespace Framebuffer {
		handle<framebuffer> Create();
		void Destroy(handle<framebuffer> framebuffer);
		handle<texture_view> ColorView(handle<framebuffer> framebuffer, u32 slot);
		void ColorView(handle<framebuffer> framebuffer, u32 slot, handle<texture_view> view);
		void DepthView(handle<framebuffer> framebuffer, handle<texture_view> view);
	} // namespace Framebuffer
} // namespace lf

#endif /* LEAF_GRAPHICS_FRAMEBUFFER_HPP */
