#pragma once

#include "resource.hpp"

#include "leaf/core/vector.hpp"
#include "leaf/graphics/command_buffer.hpp"

struct FramebufferVK : Resource {

};

namespace Framebuffer {
	lf::handle<lf::framebuffer> Create(lf::view<const lf::window> wnd);
	void Destroy(lf::handle<lf::framebuffer> fb);
	void Submit(lf::view<lf::framebuffer> fb, lf::view<const lf::command_buffer> cmd);
	void Flush(lf::view<lf::framebuffer> fb);
}
