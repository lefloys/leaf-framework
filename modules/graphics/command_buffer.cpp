#include "leaf/graphics/command_buffer.hpp"

namespace rt::Cmd {
	handle<command_buffer> Create() {
		rt_command_buffer command_buffer = rtCommandBufferCreate();
		detail::check_rutile_error("failed to create command buffer");
		return { command_buffer };
	}

	void Destroy(handle<command_buffer> command_buffer) {
		rtCommandBufferDestroy(command_buffer);
	}

	void Begin(view<command_buffer> command_buffer, view<queue> queue) {
		rtCmdBegin(command_buffer, queue);
		detail::check_rutile_error("failed to begin command buffer");
	}

	void BeginRendering(view<command_buffer> command_buffer, view<framebuffer> framebuffer) {
		rtCmdBeginRendering(command_buffer, framebuffer);
		detail::check_rutile_error("failed to begin rendering");
	}

	void ClearColor(view<command_buffer> command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a) {
		rtCmdClearColor(command_buffer, color_index, r, g, b, a);
		detail::check_rutile_error("failed to clear color");
	}

	void ClearDepth(view<command_buffer> command_buffer, f32 depth) {
		rtCmdClearDepth(command_buffer, depth);
		detail::check_rutile_error("failed to clear depth");
	}

	void ClearStencil(view<command_buffer> command_buffer, u32 stencil) {
		rtCmdClearStencil(command_buffer, stencil);
		detail::check_rutile_error("failed to clear stencil");
	}

	void UseGraphicsProgram(view<command_buffer> command_buffer, view<graphics_program> program) {
		rtCmdUseGraphicsProgram(command_buffer, program);
		detail::check_rutile_error("failed to bind graphics program");
	}

	void SetScissor(view<command_buffer> command_buffer, u32 x, u32 y, u32 width, u32 height) {
		rtCmdSetScissor(command_buffer, x, y, width, height);
		detail::check_rutile_error("failed to set scissor");
	}

	void UniformBuffer(view<command_buffer> command_buffer, uniform_location location, view<buffer> buffer, u64 offset, u64 size) {
		rtCmdUniformBuffer(command_buffer, location, buffer, offset, size);
		detail::check_rutile_error("failed to bind uniform buffer");
	}

	void UniformTexture(view<command_buffer> command_buffer, uniform_location location, view<texture_view> texture_view) {
		rtCmdUniformTexture(command_buffer, location, texture_view);
		detail::check_rutile_error("failed to bind uniform texture");
	}

	void StorageBuffer(view<command_buffer> command_buffer, uniform_location location, view<buffer> buffer, u64 offset, u64 size) {
		rtCmdStorageBuffer(command_buffer, location, buffer, offset, size);
		detail::check_rutile_error("failed to bind storage buffer");
	}

	void BindVertexBuffer(view<command_buffer> command_buffer, view<buffer> buffer, u64 offset) {
		rtCmdBindVertexBuffer(command_buffer, buffer, offset);
		detail::check_rutile_error("failed to bind vertex buffer");
	}

	void Draw(view<command_buffer> command_buffer, u32 vertex_count, u32 first_vertex) {
		rtCmdDraw(command_buffer, vertex_count, first_vertex);
		detail::check_rutile_error("failed to draw");
	}

	void EndRendering(view<command_buffer> command_buffer) {
		rtCmdEndRendering(command_buffer);
		detail::check_rutile_error("failed to end rendering");
	}

	void End(view<command_buffer> command_buffer) {
		rtCmdEnd(command_buffer);
		detail::check_rutile_error("failed to end command buffer");
	}
} // namespace rt::Cmd
