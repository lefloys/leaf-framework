#include "framebuffer_vk.hpp"

#include "../vulkan_context.hpp"

namespace Framebuffer {
	lf::handle<lf::framebuffer> create(vulkan_context& ctx, const WindowVK& wnd) {
		(void)wnd;
		return ctx.framebuffers.create();
	}
	lf::handle<lf::framebuffer> Create(lf::view<const lf::window> wnd) {
		assert_context();
		return create(get_context(), unhandle(get_context(), wnd));
	}

	void destroy(vulkan_context& ctx, lf::handle<lf::framebuffer> fb) {
		ctx.framebuffers.destroy(fb);
	}
	void Destroy(lf::handle<lf::framebuffer> fb) {
		destroy(get_context(), fb);
	}

	void submit(vulkan_context& ctx, FramebufferVK& fb, const CommandBufferVK& cmd) {
		(void)ctx;
		(void)fb;
		(void)cmd;
	}
	void Submit(lf::view<lf::framebuffer> fb, lf::view<const lf::command_buffer> cmd) {
		assert_context();
		submit(get_context(), unhandle(get_context(), fb), unhandle(get_context(), cmd));
	}

	void flush(vulkan_context& ctx, FramebufferVK& fb) {
		(void)ctx;
		(void)fb;
	}
	void Flush(lf::view<lf::framebuffer> fb) {
		assert_context();
		flush(get_context(), unhandle(get_context(), fb));
	}
}
