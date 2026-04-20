#include "command_buffer_vk.hpp"

#include "../vulkan_context.hpp"

#include <cassert>

namespace CommandBuffer {
	lf::handle<lf::command_buffer> create(vulkan_context& ctx) {
		return ctx.command_buffers.create(ctx);
	}
	lf::handle<lf::command_buffer> Create() {
		assert_context();
		return create(get_context());
	}


	void destroy(vulkan_context& ctx, lf::handle<lf::command_buffer> cmd) {
		ctx.command_buffers.destroy(cmd);
	}
	void Destroy(lf::handle<lf::command_buffer> cmd) {
		assert_context();
		assert(cmd);
		destroy(get_context(), cmd);
	}

	void reset(vulkan_context& ctx, CommandBufferVK& cmd) {
		(void)ctx;
		if (cmd.recording) {
			lf::abort();
		}
		if (VkResult result = vkResetCommandBuffer(cmd.vk_command_buffer, 0); result != VK_SUCCESS) {
			lf::abort();
		}
		cmd.ended = false;
	}
	void Reset(lf::view<lf::command_buffer> cmd) {
		assert_context();
		assert(cmd);
		reset(get_context(), unhandle(get_context(), cmd));
	}

	void begin(vulkan_context& ctx, CommandBufferVK& cmd) {
		(void)ctx;
		if (cmd.recording) {
			lf::abort();
		}

		VkCommandBufferInheritanceInfo inheritance_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };

		VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.pInheritanceInfo = &inheritance_info;

		if (VkResult result = vkBeginCommandBuffer(cmd.vk_command_buffer, &begin_info); result != VK_SUCCESS) {
			lf::abort();
		}

		cmd.recording = true;
		cmd.ended = false;
	}
	void Begin(lf::view<lf::command_buffer> cmd) {
		assert_context();
		assert(cmd);
		begin(get_context(), unhandle(get_context(), cmd));
	}

	void end(vulkan_context& ctx, CommandBufferVK& cmd) {
		(void)ctx;
		if (!cmd.recording) {
			lf::abort();
		}
		if (VkResult result = vkEndCommandBuffer(cmd.vk_command_buffer); result != VK_SUCCESS) {
			lf::abort();
		}

		cmd.recording = false;
		cmd.ended = true;
	}
	void End(lf::view<lf::command_buffer> cmd) {
		assert_context();
		assert(cmd);
		end(get_context(), unhandle(get_context(), cmd));
	}

	void draw(vulkan_context& ctx, CommandBufferVK& cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
	}
	void Draw(lf::view<lf::command_buffer> cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
		assert_context();
		assert(cmd);
		draw(get_context(), unhandle(get_context(), cmd), vertex_count, instance_count, first_vertex, first_instance);
	}
}

CommandBufferVK::CommandBufferVK(vulkan_context& ctx) : ctx(ctx) {}

CommandBufferVK::~CommandBufferVK() {}
