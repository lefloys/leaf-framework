#pragma once

#include "resource.hpp"

#include "leaf/graphics/command_buffer.hpp"

struct CommandBufferVK : Resource {
};

namespace CommandBuffer {
	lf::handle<lf::command_buffer> Create();
	void Destroy(lf::handle<lf::command_buffer> cmd);

	void Reset(lf::view<lf::command_buffer> cmd);
	void Begin(lf::view<lf::command_buffer> cmd);
	void End(lf::view<lf::command_buffer> cmd);

	void Draw(lf::view<lf::command_buffer> cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
}
