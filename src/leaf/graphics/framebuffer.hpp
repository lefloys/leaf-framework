#pragma once

#include "detail/resource.hpp"

namespace lf::Framebuffer {
	enum class status : u08 {
		complete,
		incomplete_attachment,
		incomplete_missing_attachment,
		incomplete_draw_buffer,
		incomplete_read_buffer,
		timeout,
		error
	};

	void Destroy(handle<framebuffer> fb);
	void AttachColor(view<framebuffer> fb, u32 attachment_index, view<texture_2d> color_attachment);
	void AttachDepth(view<framebuffer> fb, view<texture_2d> depth_attachment);
	void AttachStencil(view<framebuffer> fb, view<texture_2d> stencil_attachment);

	status GetStatus(view<framebuffer> fb);

}
