#ifndef LEAF_GRAPHICS_FRAMEBUFFER_HPP
#define LEAF_GRAPHICS_FRAMEBUFFER_HPP

#include <leaf/graphics/resource.hpp>

// @GPT : Again why two namespaces. why not just rt::Framebuffer
namespace rt {
	namespace Framebuffer {
		handle<framebuffer> Create();
		void Destroy(handle<framebuffer> framebuffer);
		// @GPT : You need to define them as Set. Get is implicit. Set is explicit
		view<texture_view> ColorView(view<framebuffer> framebuffer, u32 slot);
		void ColorView(view<framebuffer> framebuffer, u32 slot, view<texture_view> attachment);
		void DepthView(view<framebuffer> framebuffer, view<texture_view> attachment);
	} // namespace Framebuffer
} // namespace rt

#endif /* LEAF_GRAPHICS_FRAMEBUFFER_HPP */
