#pragma once

#include "detail/resource.hpp"

namespace lf::CommandContext {
	handle<command_context> Create();
	void Destroy(handle<command_context> cmd_ctx);

	void Wait(view<command_context> cmd_ctx, QueueTimepoint tp);
	void BeginRendering(view<command_context> cmd_ctx, view<framebuffer> fb);
	void EndRendering(view<command_context> cmd_ctx);
	void Execute(view<command_context> cmd_ctx, view<const command_buffer> cmd_buf_buf);
}
