#ifndef LEAF_GRAPHICS_COMMAND_BUFFER_HPP
#define LEAF_GRAPHICS_COMMAND_BUFFER_HPP

#include <leaf/graphics/resource.hpp>

namespace rt::CommandBuffer {
	handle<command_buffer> Create();
}

namespace rt::Cmd {
	void Destroy(handle<command_buffer> command_buffer);
	void Reset(view<command_buffer> command_buffer);
	void Begin(view<command_buffer> command_buffer);
	void Continue(view<command_buffer> command_buffer);
	void ContinueRendering(view<command_buffer> command_buffer);
	void BeginRendering(view<command_buffer> command_buffer, view<framebuffer> framebuffer);
	void ClearColor(view<command_buffer> command_buffer, location location, f32 r, f32 g, f32 b, f32 a);
	void ClearDepth(view<command_buffer> command_buffer, f32 depth);
	void ClearStencil(view<command_buffer> command_buffer, u64 stencil);
	void Clear(view<command_buffer> command_buffer, rt_clear_flag attachments);
	void UseProgram(view<command_buffer> command_buffer, view<program> program);
	void SetViewport(view<command_buffer> command_buffer, u64 x, u64 y, u64 width, u64 height, f32 min_depth, f32 max_depth);
	void SetScissor(view<command_buffer> command_buffer, u32 x, u32 y, u32 width, u32 height);
	void UniformData(view<command_buffer> command_buffer, location location, const u08* data, u64 size);
	void StorageData(view<command_buffer> command_buffer, location location, const u08* data, u64 size);
	void BindBuffer(view<command_buffer> command_buffer, location location, view<buffer> buffer, rt_buffer_range range);
	void VertexBuffer(view<command_buffer> command_buffer, location location, view<buffer> buffer, rt_buffer_range range);
	void BindTexture(view<command_buffer> command_buffer, location location, view<texture_view> texture_view);
	void BindSampler(view<command_buffer> command_buffer, location location, view<sampler> sampler);
	void BufferData(view<command_buffer> command_buffer, view<buffer> buffer, rt_buffer_range range, const u08* data);
	void TextureData(view<command_buffer> command_buffer, view<texture> texture, rt_texture_range range, const u08* data);
	void Draw(view<command_buffer> command_buffer, u32 vertex_count, u32 first_vertex);
	void EndRendering(view<command_buffer> command_buffer);
	void End(view<command_buffer> command_buffer);
} // namespace rt::Cmd

namespace rt::CommandBuffer {
	using namespace Cmd;
}

#endif /* LEAF_GRAPHICS_COMMAND_BUFFER_HPP */
