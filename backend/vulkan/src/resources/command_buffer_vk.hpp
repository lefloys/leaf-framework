#pragma once

#include "resource.hpp"

#include "leaf/graphics/command_buffer.hpp"
#include <vulkan/vulkan.h>

struct vulkan_context;

struct CommandBufferVK : Resource {
	vulkan_context& ctx;
	VkCommandBuffer vk_command_buffer = VK_NULL_HANDLE;
	bool recording = false;
	bool ended = false;

	CommandBufferVK(vulkan_context& ctx);
	~CommandBufferVK();
};

namespace CommandBuffer {
	lf::handle<lf::command_buffer> Create();
	void Destroy(lf::handle<lf::command_buffer> cmd);

	void Reset(lf::view<lf::command_buffer> cmd);
	void Begin(lf::view<lf::command_buffer> cmd);
	void End(lf::view<lf::command_buffer> cmd);

	void Draw(lf::view<lf::command_buffer> cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
}
