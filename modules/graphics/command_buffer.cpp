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

	void Reset(view<command_buffer> command_buffer) {
		rtCommandBufferReset(command_buffer);
		detail::check_rutile_error("failed to reset command buffer");
	}

	void Begin(view<command_buffer> command_buffer) {
		rtCommandBufferBegin(command_buffer);
		detail::check_rutile_error("failed to begin command buffer");
	}

	void Continue(view<command_buffer> command_buffer) {
		rtCommandBufferContinue(command_buffer);
		detail::check_rutile_error("failed to continue command buffer");
	}

	void ContinueRendering(view<command_buffer> command_buffer) {
		rtCommandBufferContinueRendering(command_buffer);
		detail::check_rutile_error("failed to continue rendering command buffer");
	}

	void BeginRendering(view<command_buffer> command_buffer, view<framebuffer> framebuffer) {
		rtCmdBeginRendering(command_buffer, framebuffer);
		detail::check_rutile_error("failed to begin rendering");
	}

	void ClearColor(view<command_buffer> command_buffer, location location, f32 r, f32 g, f32 b, f32 a) {
		rtCmdClearColor(command_buffer, location, r, g, b, a);
		detail::check_rutile_error("failed to clear color");
	}

	void ClearDepth(view<command_buffer> command_buffer, f32 depth) {
		rtCmdClearDepth(command_buffer, depth);
		detail::check_rutile_error("failed to clear depth");
	}

	void ClearStencil(view<command_buffer> command_buffer, u64 stencil) {
		rtCmdClearStencil(command_buffer, stencil);
		detail::check_rutile_error("failed to clear stencil");
	}

	void Clear(view<command_buffer> command_buffer, rt_clear_flag attachments) {
		rtCmdClear(command_buffer, attachments);
		detail::check_rutile_error("failed to clear attachments");
	}

	void UseProgram(view<command_buffer> command_buffer, view<program> program) {
		rtCmdUseProgram(command_buffer, program);
		detail::check_rutile_error("failed to bind program");
	}

	void SetViewport(view<command_buffer> command_buffer, u64 x, u64 y, u64 width, u64 height, f32 min_depth, f32 max_depth) {
		rtCmdSetViewport(command_buffer, x, y, width, height, min_depth, max_depth);
		detail::check_rutile_error("failed to set viewport");
	}

	void SetScissor(view<command_buffer> command_buffer, u32 x, u32 y, u32 width, u32 height) {
		rtCmdSetScissor(command_buffer, x, y, width, height);
		detail::check_rutile_error("failed to set scissor");
	}

	void UniformData(view<command_buffer> command_buffer, location location, const u08* data, u64 size) {
		rtCmdUniformData(command_buffer, location, data, size);
		detail::check_rutile_error("failed to set uniform data");
	}

	void StorageData(view<command_buffer> command_buffer, location location, const u08* data, u64 size) {
		rtCmdStorageData(command_buffer, location, data, size);
		detail::check_rutile_error("failed to set storage data");
	}

	void BindBuffer(view<command_buffer> command_buffer, location location, view<buffer> buffer, rt_buffer_range range) {
		rtCmdBindBuffer(command_buffer, location, buffer, range);
		detail::check_rutile_error("failed to bind buffer");
	}

	void VertexBuffer(view<command_buffer> command_buffer, location location, view<buffer> buffer, rt_buffer_range range) {
		rtCmdVertexBuffer(command_buffer, location, buffer, range);
		detail::check_rutile_error("failed to bind vertex buffer");
	}

	void BindTexture(view<command_buffer> command_buffer, location location, view<texture_view> texture_view) {
		rtCmdBindTexture(command_buffer, location, texture_view);
		detail::check_rutile_error("failed to bind texture");
	}

	void BindSampler(view<command_buffer> command_buffer, location location, view<sampler> sampler) {
		rtCmdBindSampler(command_buffer, location, sampler);
		detail::check_rutile_error("failed to bind sampler");
	}

	void BufferData(view<command_buffer> command_buffer, view<buffer> buffer, rt_buffer_range range, const u08* data) {
		rtCmdBufferData(command_buffer, buffer, range, data);
		detail::check_rutile_error("failed to upload buffer data");
	}

	void TextureData(view<command_buffer> command_buffer, view<texture> texture, rt_texture_range range, const u08* data) {
		rtCmdTextureData(command_buffer, texture, range, data);
		detail::check_rutile_error("failed to upload texture data");
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
		rtCommandBufferEnd(command_buffer);
		detail::check_rutile_error("failed to end command buffer");
	}
} // namespace rt::Cmd
