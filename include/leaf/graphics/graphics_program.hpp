#ifndef LEAF_GRAPHICS_GRAPHICS_PROGRAM_HPP
#define LEAF_GRAPHICS_GRAPHICS_PROGRAM_HPP

#include <leaf/core/span.hpp>
#include <leaf/core/string.hpp>
#include <leaf/graphics/format.hpp>

namespace rt {
	enum class CullMode : u32 {
		None = 0,
		Front = 1,
		Back = 2,
	};

	enum class FrontFace : u32 {
		CounterClockwise = 0,
		Clockwise = 1,
	};

	enum class FillMode : u32 {
		Solid = 0,
		Wireframe = 1,
	};

	struct vertex_attribute {
		const char* name = nullptr;
		u32 offset = 0;
		Format format = Format::Unknown;
	};

	struct vertex_input {
		const vertex_attribute* attributes = nullptr;
		u32 attribute_count = 0;
		u32 stride = 0;
		rt_vertex_rate rate = RT_VERTEX_RATE_VERTEX;
	};

	struct vertex_layout {
		const vertex_input* inputs = nullptr;
		u32 input_count = 0;
	};

	namespace Program {
		handle<program> Create();
		void Destroy(handle<program> program);
		void VertexLayout(view<program> program, const vertex_layout& layout);
		void Source(view<program> program, string_view entry_point, span<const byte> data);
		void RasterState(view<program> program, CullMode cull_mode, FrontFace front_face, FillMode fill_mode);
		void BlendState(view<program> program, bool enabled, rt_blend_factor src_color, rt_blend_factor dst_color, rt_blend_op color_op, rt_blend_factor src_alpha, rt_blend_factor dst_alpha, rt_blend_op alpha_op);
		void Finalize(view<program> program);
		location UniformLocation(view<program> program, string_view name);
		location InputLocation(view<program> program, span<const vertex_attribute> attributes);
		location OutputLocation(view<program> program, string_view name = {});
	} // namespace Program

} // namespace rt

#endif /* LEAF_GRAPHICS_GRAPHICS_PROGRAM_HPP */
