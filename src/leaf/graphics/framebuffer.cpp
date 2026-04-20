#include "framebuffer.hpp"
#include "detail/api.hpp"

namespace lf {
	void Framebuffer::Destroy(handle<framebuffer> fb) {
		Graphics.Framebuffer.destroy(fb);
	}

	void Framebuffer::Submit(view<framebuffer> fb, view<const command_buffer> cmd) {
		Graphics.Framebuffer.submit(fb, cmd);
	}

	void Framebuffer::Flush(view<framebuffer> fb) {
		Graphics.Framebuffer.flush(fb);
	}
}
