#pragma once

#include "detail/resource.hpp"

namespace lf::Framebuffer {
	handle<framebuffer> Create(view<const window> wnd);
	void Destroy(handle<framebuffer> fb);
	void Submit(view<framebuffer> fb, view<const command_buffer> cmd);
	void Flush(view<framebuffer> fb);
}
