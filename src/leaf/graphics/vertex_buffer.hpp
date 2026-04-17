#pragma once

#include "detail/resource.hpp"

namespace lf::VertexBuffer {
	handle<vertex_buffer> Create(buffer_usage usage, u64 size, const void* data);
	void Destroy(handle<vertex_buffer> buf);
}
