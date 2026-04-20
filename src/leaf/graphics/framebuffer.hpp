#pragma once

#include "detail/resource.hpp"

namespace lf::Framebuffer {
	void Destroy(handle<framebuffer> fb);
	void Submit(view<framebuffer> fb, view<const command_buffer> cmd);
	void Flush(view<framebuffer> fb);
}
