#pragma once

#include "resource.hpp"

#include <leaf/core/vector.hpp>
#include <leaf/graphics/command_buffer.hpp>

struct FramebufferVK : Resource {
	// this is the primary command buffer that will be submitted for rendering
	// Everything else is submitted to it. by calling flush it is submitted.
	// It is reset every time it gets submitted.
	VkCommandBuffer vk_command_buffer;
};

namespace Framebuffer {
	lf::handle<lf::framebuffer> Create(lf::view<const lf::window> wnd);
	void Destroy(lf::handle<lf::framebuffer> fb);
	void Submit(lf::view<lf::framebuffer> fb, lf::view<const lf::command_buffer> cmd);
	void Flush(lf::view<lf::framebuffer> fb);
}
