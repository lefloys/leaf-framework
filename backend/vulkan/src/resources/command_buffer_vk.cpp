#include "command_buffer_vk.hpp"

#include "../vulkan_context.hpp"

#include <cassert>

namespace CommandBuffer {
	lf::handle<lf::command_buffer> create() {
		assert_context();
		return get_context().command_buffers.create();
	}
	lf::handle<lf::command_buffer> Create() {
		return create();
	}


	void destroy(lf::handle<lf::command_buffer> cmd) {
		get_context().command_buffers.destroy(cmd);
	}
	void Destroy(lf::handle<lf::command_buffer> cmd) {
		assert_context();
		assert(cmd);
		destroy(cmd);
	}

	void reset(CommandBufferVK& cmd) {
	}
	void Reset(lf::view<lf::command_buffer> cmd) {
		assert_context();
		assert(cmd);
		reset(unhandle(cmd));
	}

	void begin(CommandBufferVK& cmd) {
	}
	void Begin(lf::view<lf::command_buffer> cmd) {
		assert_context();
		assert(cmd);
		begin(unhandle(cmd));
	}

	void end(CommandBufferVK& cmd) {
	}
	void End(lf::view<lf::command_buffer> cmd) {
		assert_context();
		assert(cmd);
		end(unhandle(cmd));
	}

	void draw(CommandBufferVK& cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
	}
	void Draw(lf::view<lf::command_buffer> cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
		assert_context();
		assert(cmd);
		draw(unhandle(cmd), vertex_count, instance_count, first_vertex, first_instance);
	}

}
