#include "command_buffer.hpp"

#include <leaf/core/exception.hpp>

namespace lf {
	void RequireCommandBufferComputeExtension() {
		if (!rt_rtCmdUseComputeProgram) {
			throw runtime_exception("Rutile compute extension is not available");
		}
	}

	handle<command_buffer> CommandBuffer::Create() {
		rt_command_buffer command_buffer = rtCmdCreate();
		detail::check_rutile_error("failed to create command buffer");
		return { command_buffer };
	}

	void CommandBuffer::Destroy(handle<command_buffer> command_buffer) {
		rtCmdDestroy(command_buffer);
	}

	void CommandBuffer::Begin(view<command_buffer> command_buffer, view<queue> queue) {
		rtCmdBegin(command_buffer, queue);
		detail::check_rutile_error("failed to begin command buffer");
	}

	void CommandBuffer::BeginRendering(view<command_buffer> command_buffer,
									   view<framebuffer> framebuffer) {
		rtCmdBeginRendering(command_buffer, framebuffer);
		detail::check_rutile_error("failed to begin rendering");
	}

	void CommandBuffer::ClearColor(view<command_buffer> command_buffer, u32 color_index, f32 r,
								   f32 g, f32 b, f32 a) {
		rtCmdClearColor(command_buffer, color_index, r, g, b, a);
		detail::check_rutile_error("failed to clear color");
	}

	void CommandBuffer::ClearDepth(view<command_buffer> command_buffer, f32 depth) {
		rtCmdClearDepth(command_buffer, depth);
		detail::check_rutile_error("failed to clear depth");
	}

	void CommandBuffer::ClearStencil(view<command_buffer> command_buffer, u32 stencil) {
		rtCmdClearStencil(command_buffer, stencil);
		detail::check_rutile_error("failed to clear stencil");
	}

	void CommandBuffer::UseGraphicsProgram(view<command_buffer> command_buffer,
										   view<graphics_program> program) {
		rtCmdUseGraphicsProgram(command_buffer, program);
		detail::check_rutile_error("failed to bind graphics program");
	}

	void CommandBuffer::SetScissor(view<command_buffer> command_buffer, u32 x, u32 y, u32 width,
								   u32 height) {
		rtCmdSetScissor(command_buffer, x, y, width, height);
		detail::check_rutile_error("failed to set scissor");
	}

	void CommandBuffer::UseComputeProgram(view<command_buffer> command_buffer,
										  view<compute_program> program) {
		RequireCommandBufferComputeExtension();
		rtCmdUseComputeProgram(command_buffer, program);
		detail::check_rutile_error("failed to bind compute program");
	}

	void CommandBuffer::UniformBuffer(view<command_buffer> command_buffer,
									  uniform_location location, view<buffer> buffer,
									  u64 offset, u64 size) {
		rtCmdUniformBuffer(command_buffer, location, buffer, offset, size);
		detail::check_rutile_error("failed to bind uniform buffer");
	}

	void CommandBuffer::UniformTexture(view<command_buffer> command_buffer,
									   uniform_location location,
									   view<texture_view> texture_view) {
		rtCmdUniformTexture(command_buffer, location, texture_view);
		detail::check_rutile_error("failed to bind uniform texture");
	}

	void CommandBuffer::StorageBuffer(view<command_buffer> command_buffer, u32 binding,
									  view<buffer> buffer, u64 offset, u64 size) {
		if (!rt_rtCmdStorageBuffer) {
			rt_rtCmdStorageBuffer = (PFN_rtCmdStorageBuffer)rtGetProc("rtCmdStorageBuffer");
		}
		if (!rt_rtCmdStorageBuffer) {
			throw runtime_exception("Rutile storage buffer binding is not available");
		}
		rtCmdStorageBuffer(command_buffer, binding, buffer, offset, size);
		detail::check_rutile_error("failed to bind storage buffer");
	}

	void CommandBuffer::StorageTexture(view<command_buffer> command_buffer, u32 binding,
									   view<texture_view> texture_view) {
		RequireCommandBufferComputeExtension();
		rtCmdStorageTexture(command_buffer, binding, texture_view);
		detail::check_rutile_error("failed to bind storage texture");
	}

	void CommandBuffer::ComputeBarrier(view<command_buffer> command_buffer) {
		RequireCommandBufferComputeExtension();
		rtCmdComputeBarrier(command_buffer);
		detail::check_rutile_error("failed to insert compute barrier");
	}

	void CommandBuffer::BindVertexBuffer(view<command_buffer> command_buffer, view<buffer> buffer,
										 u64 offset) {
		rtCmdBindVertexBuffer(command_buffer, buffer, offset);
		detail::check_rutile_error("failed to bind vertex buffer");
	}

	void CommandBuffer::Draw(view<command_buffer> command_buffer, u32 vertex_count,
							 u32 first_vertex) {
		rtCmdDraw(command_buffer, vertex_count, first_vertex);
		detail::check_rutile_error("failed to draw");
	}

	void CommandBuffer::Dispatch(view<command_buffer> command_buffer, u32 group_count_x,
								 u32 group_count_y, u32 group_count_z) {
		RequireCommandBufferComputeExtension();
		rtCmdDispatch(command_buffer, group_count_x, group_count_y, group_count_z);
		detail::check_rutile_error("failed to dispatch");
	}

	void CommandBuffer::EndRendering(view<command_buffer> command_buffer) {
		rtCmdEndRendering(command_buffer);
		detail::check_rutile_error("failed to end rendering");
	}

	void CommandBuffer::End(view<command_buffer> command_buffer) {
		rtCmdEnd(command_buffer);
		detail::check_rutile_error("failed to end command buffer");
	}
} // namespace lf
