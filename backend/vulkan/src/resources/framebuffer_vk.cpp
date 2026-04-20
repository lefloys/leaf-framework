#include "framebuffer_vk.hpp"

#include "../vulkan_context.hpp"

#include "leaf/core/exception.hpp"

namespace Framebuffer {
	lf::handle<lf::framebuffer> create(vulkan_context& ctx, const WindowVK& wnd) {
		(void)ctx;
		if (!wnd.active_framebuffer) {
			throw lf::runtime_exception("Framebuffer::Create was called without an active frame (missing BeginFrame)");
		}
		return wnd.active_framebuffer;
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
		if (!cmd.ended) {
			lf::abort();
		}
		fb.submitted_secondary_buffers.push_back(cmd.vk_command_buffer);
	}
	void Submit(lf::view<lf::framebuffer> fb, lf::view<const lf::command_buffer> cmd) {
		assert_context();
		submit(get_context(), unhandle(get_context(), fb), unhandle(get_context(), cmd));
	}

	void flush(vulkan_context& ctx, FramebufferVK& fb) {
		(void)ctx;
		// Frame submission is coordinated by the window frame lifecycle (AcquireImage/Present),
		// because the framebuffer itself does not own swapchain images or presentation state.
		fb.submitted_secondary_buffers.clear();
	}
	void Flush(lf::view<lf::framebuffer> fb) {
		assert_context();
		flush(get_context(), unhandle(get_context(), fb));
	}
}

FramebufferVK::FramebufferVK(vulkan_context& ctx)
	: ctx(ctx) {
	const QueueVK& graphics_queue = unhandle(ctx, lf::view<const lf::queue>(ctx.graphics_queue));
	VkCommandPoolCreateInfo pool_create_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	pool_create_info.queueFamilyIndex = graphics_queue.family_index;
	pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (VkResult result = vkCreateCommandPool(ctx.vk_device, &pool_create_info, nullptr, &vk_command_pool); result != VK_SUCCESS) {
		lf::abort();
	}

	VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	alloc_info.commandPool = vk_command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1;
	if (VkResult result = vkAllocateCommandBuffers(ctx.vk_device, &alloc_info, &vk_primary_command_buffer); result != VK_SUCCESS) {
		lf::abort();
	}
}

FramebufferVK::~FramebufferVK() {
	vk_primary_command_buffer = VK_NULL_HANDLE;
	if (vk_command_pool) {
		vkDestroyCommandPool(ctx.vk_device, vk_command_pool, nullptr);
		vk_command_pool = VK_NULL_HANDLE;
	}
}
