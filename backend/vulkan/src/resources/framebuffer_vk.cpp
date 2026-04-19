#include "framebuffer_vk.hpp"

#include "../vulkan_context.hpp"

namespace Framebuffer {
	lf::handle<lf::framebuffer> Create(lf::view<const lf::window> wnd) {
		assert_context();
		return get_context().framebuffers.create();
	}

	void Destroy(lf::handle<lf::framebuffer> fb) {
		get_context().framebuffers.destroy(fb);
	}

	void Submit(lf::view<lf::framebuffer> fb, lf::view<const lf::command_buffer> cmd) {
	}

	void Flush(lf::view<lf::framebuffer> fb) {
	}
}
