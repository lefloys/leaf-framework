#include "leaf/graphics/graphics_program.hpp"

#include <utility>

namespace rt::detail {
	rt_cull_mode to_rutile(CullMode mode) {
		switch (mode) {
		case CullMode::None: return RT_CULL_NONE;
		case CullMode::Front: return RT_CULL_FRONT;
		case CullMode::Back: return RT_CULL_BACK;
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
		std::unreachable();
	}
} // namespace rt::detail

namespace rt {
	handle<program> Program::Create() {
		rt_program program = rtProgramCreate();
		detail::check_rutile_error("failed to create program");
		return { program };
	}

	void Program::Destroy(handle<program> program) {
		rtProgramDestroy(program);
	}

	void Program::VertexLayout(view<program> program, const vertex_layout& layout) {
		vector<rt_vertex_input> inputs;
		vector<vector<rt_vertex_attribute>> attributes;
		inputs.reserve(layout.input_count);
		attributes.reserve(layout.input_count);
		for (u32 input_index = 0; input_index < layout.input_count; ++input_index) {
			const vertex_input& input = layout.inputs[input_index];
			vector<rt_vertex_attribute>& native_attributes = attributes.emplace_back();
			native_attributes.reserve(input.attribute_count);
			for (u32 attribute_index = 0; attribute_index < input.attribute_count; ++attribute_index) {
				const vertex_attribute& attribute = input.attributes[attribute_index];
				native_attributes.push_back({ attribute.name, attribute.offset, static_cast<rt_format>(attribute.format) });
			}
			inputs.push_back({ native_attributes.data(), native_attributes.size(), input.stride, input.rate });
		}
		rt_vertex_layout rutile_layout = { inputs.data(), inputs.size() };
		rtProgramSetLayout(program, &rutile_layout);
		detail::check_rutile_error("failed to set program vertex layout");
	}

	void Program::Source(view<program> program, string_view entry_point, span<const byte> data) {
		rtProgramSource(program, string(entry_point).c_str(), reinterpret_cast<const u08*>(data.data()), data.size());
		detail::check_rutile_error("failed to set program source");
	}

	void Program::RasterState(view<program> program, CullMode cull_mode, FrontFace front_face, FillMode fill_mode) {
		rtProgramSetRasterState(program, detail::to_rutile(cull_mode), detail::to_rutile(front_face), detail::to_rutile(fill_mode));
		detail::check_rutile_error("failed to set program raster state");
	}

	void Program::BlendState(view<program> program, bool enabled, rt_blend_factor src_color, rt_blend_factor dst_color, rt_blend_op color_op, rt_blend_factor src_alpha, rt_blend_factor dst_alpha, rt_blend_op alpha_op) {
		rtProgramSetBlendState(program, enabled, src_color, dst_color, color_op, src_alpha, dst_alpha, alpha_op);
		detail::check_rutile_error("failed to set program blend state");
	}

	void Program::Finalize(view<program> program) {
		rtProgramFinalize(program);
		detail::check_rutile_error("failed to finalize program");
	}

	location Program::UniformLocation(view<program> program, string_view name) {
		location result = rtProgramUniformLocation(program, string(name).c_str());
		detail::check_rutile_error("failed to query program uniform location");
		return result;
	}

	location Program::InputLocation(view<program> program, span<const vertex_attribute> attributes) {
		vector<rt_vertex_attribute> native_attributes;
		native_attributes.reserve(attributes.size());
		for (const vertex_attribute& attribute : attributes) {
			native_attributes.push_back({ attribute.name, attribute.offset, static_cast<rt_format>(attribute.format) });
		}
		location result = rtProgramInputLocation(program, native_attributes.data(), native_attributes.size());
		detail::check_rutile_error("failed to query program input location");
		return result;
	}

	location Program::OutputLocation(view<program> program, string_view name) {
		location result = rtProgramOutputLocation(program, name.empty() ? nullptr : string(name).c_str());
		detail::check_rutile_error("failed to query program output location");
		return result;
	}

} // namespace rt
