#include "framebuffer.hpp"

namespace lf {
	handle<framebuffer> Framebuffer::Create() {
		rt_framebuffer framebuffer = rtFramebufferCreate();
		detail::check_rutile_error("failed to create framebuffer");
		return { framebuffer };
	}

	void Framebuffer::Destroy(handle<framebuffer> framebuffer) {
		rtFramebufferDestroy(framebuffer);
	}

	view<texture_view> Framebuffer::ColorView(view<framebuffer> framebuffer, u32 slot) {
		rt_texture_view value = rtFramebufferColorView(framebuffer, slot);
		detail::check_rutile_error("failed to get framebuffer color view");
		lf::view<texture_view> result;
		result.value = value;
		return result;
	}

	void Framebuffer::ColorView(view<framebuffer> framebuffer, u32 slot, view<texture_view> attachment) {
		rtFramebufferSetColorView(framebuffer, slot, attachment);
		detail::check_rutile_error("failed to set framebuffer color view");
	}

	void Framebuffer::DepthView(view<framebuffer> framebuffer, view<texture_view> attachment) {
		rtFramebufferDepthView(framebuffer, attachment);
		detail::check_rutile_error("failed to set framebuffer depth view");
	}
} // namespace lf
