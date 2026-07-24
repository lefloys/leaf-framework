#include "graphics_program.hpp"

#include <utility>

namespace rt::detail {
	// @GPT : Why so many to rutile. why dont you just assign the enum values to the rutile values. like "Unknown = RT_FORMAT_UNKNOWN" etc.
	// That way we dont need any of these ffs.
	rt_format to_rutile(Format format) {
		switch (format) {
		case Format::Rg32Float: return RT_RG32_SFLOAT;
		case Format::Rgb32Float: return RT_RGB32_SFLOAT;
		case Format::Rgba32Float: return RT_RGBA32_SFLOAT;
		default: return RT_FORMAT_UNKNOWN;
		}
	}

	rt_cull_mode to_rutile(CullMode mode) {
		switch (mode) {
		case CullMode::None: return RT_CULL_NONE;
		case CullMode::Front: return RT_CULL_FRONT;
		case CullMode::Back: return RT_CULL_BACK;
			// @GPT : why not default std::unreachable. why later
		}
		std::unreachable();
	}

	rt_front_face to_rutile(FrontFace face) {
		switch (face) {
		case FrontFace::CounterClockwise: return RT_FRONT_FACE_CCW;
		case FrontFace::Clockwise: return RT_FRONT_FACE_CW;
		}
		std::unreachable();
	}

	rt_fill_mode to_rutile(FillMode mode) {
		switch (mode) {
		case FillMode::Solid: return RT_FILL_SOLID;
		case FillMode::Wireframe: return RT_FILL_WIREFRAME;
		}
		// @GPT : same fucking thing.
		std::unreachable();
	}
} // namespace rt::detail

namespace rt {
	handle<graphics_program> GraphicsProgram::Create() {
		rt_graphics_program program = rtGraphicsProgramCreate();
		detail::check_rutile_error("failed to create graphics program");
		return { program };
	}

	void GraphicsProgram::Destroy(handle<graphics_program> program) {
		rtGraphicsProgramDestroy(program);
	}

	void GraphicsProgram::VertexLayout(view<graphics_program> program, const vertex_layout& layout) {
		rt_vertex_attribute attributes[16] = {};
		const u32 attribute_count = layout.attribute_count > 16 ? 16 : layout.attribute_count;
		for (u32 i = 0; i < attribute_count; ++i) {
			attributes[i] = { layout.attributes[i].name, layout.attributes[i].offset,
							  detail::to_rutile(layout.attributes[i].format) };
		}

		rt_vertex_layout rutile_layout = { layout.stride, attributes, attribute_count };
		rtGraphicsProgramLayout(program, &rutile_layout);
		detail::check_rutile_error("failed to set graphics program vertex layout");
	}

	void GraphicsProgram::Source(view<graphics_program> program, u64 size, const void* data) {
		rtGraphicsProgramSource(program, size, data);
		detail::check_rutile_error("failed to set graphics program source");
	}

	void GraphicsProgram::Source(view<graphics_program> program, span<const byte> data) {
		Source(program, data.size(), data.data());
	}

	void GraphicsProgram::RasterState(view<graphics_program> program, CullMode cull_mode, FrontFace front_face, FillMode fill_mode) {
		rtGraphicsProgramRasterState(program, detail::to_rutile(cull_mode), detail::to_rutile(front_face), detail::to_rutile(fill_mode));
		detail::check_rutile_error("failed to set graphics program raster state");
	}

	void GraphicsProgram::BlendState(view<graphics_program> program, bool enabled, rt_blend_factor src_color, rt_blend_factor dst_color, rt_blend_op color_op, rt_blend_factor src_alpha, rt_blend_factor dst_alpha, rt_blend_op alpha_op) {
		rtGraphicsProgramBlendState(program, enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op);
		detail::check_rutile_error("failed to set graphics program blend state");
	}

	void GraphicsProgram::Finalize(view<graphics_program> program) {
		rtGraphicsProgramFinalize(program);
		detail::check_rutile_error("failed to finalize graphics program");
	}

	void GraphicsProgram::Reset(view<graphics_program> program) {
		rtGraphicsProgramReset(program);
		detail::check_rutile_error("failed to reset graphics program");
	}

	uniform_location GraphicsProgram::UniformLocation(view<graphics_program> program, string_view name) {
		rt_uniform_location location = rtGraphicsProgramUniformLocation(program, string(name).c_str());
		detail::check_rutile_error("failed to query graphics program uniform location");
		return location;
	}

} // namespace rt
