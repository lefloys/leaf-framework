#include "framebuffer_vk.hpp"

#include "../vulkan_context.hpp"

#include "leaf/core/exception.hpp"

namespace Framebuffer {
	void destroy(vulkan_context& ctx, lf::handle<lf::framebuffer> fb) {
		ctx.framebuffers.destroy(fb);
	}
	void Destroy(lf::handle<lf::framebuffer> fb) {
		destroy(get_context(), fb);
	}
}

FramebufferVK::FramebufferVK(vulkan_context& ctx) : ctx(ctx) {}

FramebufferVK::~FramebufferVK() {}
