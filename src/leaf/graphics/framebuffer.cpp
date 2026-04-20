#include "framebuffer.hpp"
#include "detail/api.hpp"

namespace lf {
	void Framebuffer::Destroy(handle<framebuffer> fb) {
		Graphics.Framebuffer.destroy(fb);
	}
}
