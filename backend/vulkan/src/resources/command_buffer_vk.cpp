#include "command_buffer_vk.hpp"

#include "../vulkan_context.hpp"

#include <cassert>

namespace CommandBuffer {
	lf::handle<lf::command_buffer> create(vulkan_context& ctx) {
		return ctx.command_buffers.create();
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
		(void)cmd;
	}
	void Reset(lf::view<lf::command_buffer> cmd) {
		assert_context();
		assert(cmd);
		reset(get_context(), unhandle(get_context(), cmd));
	}

	void begin(vulkan_context& ctx, CommandBufferVK& cmd) {
		(void)ctx;
		(void)cmd;
	}
	void Begin(lf::view<lf::command_buffer> cmd) {
		assert_context();
		assert(cmd);
		begin(get_context(), unhandle(get_context(), cmd));
	}

	void end(vulkan_context& ctx, CommandBufferVK& cmd) {
		(void)ctx;
		(void)cmd;
	}
	void End(lf::view<lf::command_buffer> cmd) {
		assert_context();
		assert(cmd);
		end(get_context(), unhandle(get_context(), cmd));
	}

	void draw(vulkan_context& ctx, CommandBufferVK& cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
		(void)ctx;
		(void)cmd;
		(void)vertex_count;
		(void)instance_count;
		(void)first_vertex;
		(void)first_instance;
	}
	void Draw(lf::view<lf::command_buffer> cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
		assert_context();
		assert(cmd);
		draw(get_context(), unhandle(get_context(), cmd), vertex_count, instance_count, first_vertex, first_instance);
	}

}
