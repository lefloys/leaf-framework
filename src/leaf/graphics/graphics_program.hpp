#ifndef LEAF_GRAPHICS_GRAPHICS_PROGRAM_HPP
#define LEAF_GRAPHICS_GRAPHICS_PROGRAM_HPP

#include <leaf/graphics/format.hpp>

namespace lf {

	enum class CullMode {
		None,
		Front,
		Back,
	};

	enum class FrontFace {
		CounterClockwise,
		Clockwise,
	};

	enum class FillMode {
		Solid,
		Wireframe,
	};

	struct vertex_attribute {
		u32 location = 0;
		u32 offset = 0;
		Format format = Format::Unknown;
	};

	struct vertex_layout {
		u32 stride = 0;
		const vertex_attribute* attributes = nullptr;
		u32 attribute_count = 0;
	};

	namespace GraphicsProgram {
		handle<graphics_program> Create();
		void Destroy(handle<graphics_program> program);
		void VertexLayout(handle<graphics_program> program, const vertex_layout& layout);
		void VertexShader(handle<graphics_program> program, u64 size, const void* data);
		void FragmentShader(handle<graphics_program> program, u64 size, const void* data);
		void RasterState(handle<graphics_program> program, CullMode cull_mode, FrontFace front_face,
						 FillMode fill_mode);
		void BlendState(handle<graphics_program> program, bool enabled, rt_blend_factor src_color,
						rt_blend_factor dst_color, rt_blend_op color_op, rt_blend_factor src_alpha,
						rt_blend_factor dst_alpha, rt_blend_op alpha_op);
		void Link(handle<graphics_program> program);
		uniform_location UniformLocation(handle<graphics_program> program, const char* name);
	} // namespace GraphicsProgram

} // namespace lf

#endif /* LEAF_GRAPHICS_GRAPHICS_PROGRAM_HPP */
