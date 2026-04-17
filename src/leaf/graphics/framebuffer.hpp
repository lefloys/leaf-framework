#pragma once

#include "detail/resource.hpp"

namespace lf::Framebuffer {
	handle<framebuffer> Create(view<const window> wnd);
	void Destroy(handle<framebuffer> fb);
	view<const texture2d> GetColorAttachment(view<const framebuffer> fb);
	view<const texture2d> GetDepthAttachment(view<const framebuffer> fb);
}
