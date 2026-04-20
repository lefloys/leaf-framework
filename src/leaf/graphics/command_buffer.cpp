#include "command_buffer.hpp"
#include "detail/api.hpp"

namespace lf {
	handle<command_buffer> CommandBuffer::Create() {
		return Graphics.CommandBuffer.create();
	}

	void CommandBuffer::Destroy(handle<command_buffer> cmd_buf) {
		Graphics.CommandBuffer.destroy(cmd_buf);
	}

	void CommandBuffer::Reset(view<command_buffer> cmd_buf) {
		Graphics.CommandBuffer.reset(cmd_buf);
	}

	void CommandBuffer::Begin(view<command_buffer> cmd_buf) {
		Graphics.CommandBuffer.begin(cmd_buf);
	}

	void CommandBuffer::End(view<command_buffer> cmd_buf) {
		Graphics.CommandBuffer.end(cmd_buf);
	}

	void CommandBuffer::Draw(view<command_buffer> cmd_buf, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
		Graphics.CommandBuffer.draw(cmd_buf, vertex_count, instance_count, first_vertex, first_instance);
	}
}
