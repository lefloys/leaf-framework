#pragma once

#include "resource.hpp"

#include <leaf/core/vector.hpp>
#include <leaf/graphics/command_buffer.hpp>
#include <leaf/graphics/window.hpp>
#include <vulkan/vulkan.h>

struct vulkan_context;

struct FramebufferVK : Resource {
	vulkan_context& ctx;
	lf::vector<VkImageView> color_attachments;
	lf::vector<VkCommandBuffer> submitted_secondary_buffers;
	VkCommandPool vk_command_pool = VK_NULL_HANDLE;
	VkCommandBuffer vk_primary_command_buffer = VK_NULL_HANDLE;

	FramebufferVK(vulkan_context& ctx);
	~FramebufferVK();
};

namespace Framebuffer {
	lf::handle<lf::framebuffer> Create(lf::view<const lf::window> wnd);
	void Destroy(lf::handle<lf::framebuffer> fb);
	void Submit(lf::view<lf::framebuffer> fb, lf::view<const lf::command_buffer> cmd);
	void Flush(lf::view<lf::framebuffer> fb);
}
