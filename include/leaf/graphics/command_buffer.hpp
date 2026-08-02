#ifndef LEAF_GRAPHICS_COMMAND_BUFFER_HPP
#define LEAF_GRAPHICS_COMMAND_BUFFER_HPP

#include <leaf/graphics/resource.hpp>
namespace rt::Cmd {
	handle<command_buffer> Create();
	void Destroy(handle<command_buffer> command_buffer);
	void Begin(view<command_buffer> command_buffer, view<queue> queue);
	void BeginRendering(view<command_buffer> command_buffer, view<framebuffer> framebuffer);
	void ClearColor(view<command_buffer> command_buffer, u32 color_index, f32 r, f32 g, f32 b, f32 a);
	void ClearDepth(view<command_buffer> command_buffer, f32 depth);
	void ClearStencil(view<command_buffer> command_buffer, u32 stencil);
	void UseGraphicsProgram(view<command_buffer> command_buffer, view<graphics_program> program);
	void SetScissor(view<command_buffer> command_buffer, u32 x, u32 y, u32 width, u32 height);
	void UniformBuffer(view<command_buffer> command_buffer, uniform_location location, view<buffer> buffer, u64 offset, u64 size);
	void UniformTexture(view<command_buffer> command_buffer, uniform_location location, view<texture_view> texture_view);
	void StorageBuffer(view<command_buffer> command_buffer, uniform_location location, view<buffer> buffer, u64 offset, u64 size);
	void BindVertexBuffer(view<command_buffer> command_buffer, view<buffer> buffer, u64 offset);
	void Draw(view<command_buffer> command_buffer, u32 vertex_count, u32 first_vertex);
	void EndRendering(view<command_buffer> command_buffer);
	void End(view<command_buffer> command_buffer);
} // namespace rt::Cmd

namespace rt::CommandBuffer {
	using namespace Cmd;
}

#endif /* LEAF_GRAPHICS_COMMAND_BUFFER_HPP */
