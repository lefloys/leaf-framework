#pragma once

#include "resource.hpp"
#include "texture_vk.hpp"

#include <leaf/core/vector.hpp>
#include <leaf/graphics/command_buffer.hpp>
#include <leaf/graphics/window.hpp>
#include <vulkan/vulkan.h>

struct vulkan_context;

struct FramebufferVK : Resource {
	FramebufferVK(vulkan_context& ctx);
	~FramebufferVK();

	vulkan_context& ctx;
	lf::vector<lf::view<const lf::texture_base>> color_attachments;
};

namespace Framebuffer {
	void Destroy(lf::handle<lf::framebuffer> fb);
}
