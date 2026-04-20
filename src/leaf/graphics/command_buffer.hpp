#pragma once

#include "detail/resource.hpp"

namespace lf::CommandBuffer {
	handle<command_buffer> Create();
	void Destroy(handle<command_buffer> cmd);

	void Reset(view<command_buffer> cmd_buf);
	void Begin(view<command_buffer> cmd_buf);
	void End(view<command_buffer> cmd_buf);

	void Draw(view<command_buffer> cmd_buf, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
}