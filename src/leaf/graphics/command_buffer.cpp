#include "command_buffer.hpp"
#include "detail/api.hpp"

namespace lf {
	handle<command_buffer> CommandBuffer::Create() {
		return Graphics.CommandBuffer.create();
	}

	void CommandBuffer::Destroy(handle<command_buffer> cmd) {
		Graphics.CommandBuffer.destroy(cmd);
	}

	void CommandBuffer::Reset(view<command_buffer> cmd) {
		Graphics.CommandBuffer.reset(cmd);
	}

	void CommandBuffer::Begin(view<command_buffer> cmd) {
		Graphics.CommandBuffer.begin_recording(cmd);
	}

	void CommandBuffer::End(view<command_buffer> cmd) {
		Graphics.CommandBuffer.end_recording(cmd);
	}

	void CommandBuffer::Draw(view<command_buffer> cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
		Graphics.CommandBuffer.draw(cmd, vertex_count, instance_count, first_vertex, first_instance);
	}
}
