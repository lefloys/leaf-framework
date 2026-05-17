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

	handle<texture_view> Framebuffer::ColorView(handle<framebuffer> framebuffer, u32 slot) {
		rt_texture_view view = rtFramebufferColorView(framebuffer, slot);
		detail::check_rutile_error("failed to get framebuffer color view");
		return { view };
	}

	void Framebuffer::ColorView(handle<framebuffer> framebuffer, u32 slot, handle<texture_view> view) {
		rtFramebufferSetColorView(framebuffer, slot, view);
		detail::check_rutile_error("failed to set framebuffer color view");
	}

	void Framebuffer::DepthView(handle<framebuffer> framebuffer, handle<texture_view> view) {
		rtFramebufferDepthView(framebuffer, view);
		detail::check_rutile_error("failed to set framebuffer depth view");
	}
} // namespace lf
