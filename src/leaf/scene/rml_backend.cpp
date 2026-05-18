#include "rml_backend.hpp"

#include <leaf/core/format.hpp>

#include <RmlUi/Core.h>
#include <stb_image.h>

#include <algorithm>
#include <cstring>
#include <memory>

namespace lf {
	struct UiVertex {
		pos2<f32> position;
		pos2<f32> uv;
		f32 color[4];
	};

	struct UiUniform {
		f32 viewport_size[2] = { 1.0f, 1.0f };
		f32 translation[2] = { 0.0f, 0.0f };
		f32 texture_mode = 0.0f;
		f32 padding = 0.0f;
	};

	constexpr const char* kUiVertexShader = R"(
#version 460
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

layout(set = 0, binding = 0) uniform UiDraw {
	vec2 viewport_size;
	vec2 translation;
	float texture_mode;
	float padding;
} u_ui;

void main() {
	vec2 pixel = in_position + u_ui.translation;
	vec2 ndc = vec2(pixel.x / u_ui.viewport_size.x * 2.0 - 1.0,
					1.0 - pixel.y / u_ui.viewport_size.y * 2.0);
	gl_Position = vec4(ndc, 0.0, 1.0);
	out_uv = in_uv;
	out_color = in_color;
}
)";

	constexpr const char* kUiFragmentShader = R"(
#version 460
layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform UiDraw {
	vec2 viewport_size;
	vec2 translation;
	float texture_mode;
	float padding;
} u_ui;
layout(set = 0, binding = 1) uniform sampler2D UiTexture;

void main() {
	if (u_ui.texture_mode < 0.5) {
		out_color = in_color;
		return;
	}
	out_color = texture(UiTexture, in_uv) * in_color;
}
)";

	struct RmlRenderInterface::Geometry {
		unique<buffer> vertices;
		u32 vertex_count = 0;
	};

	struct RmlRenderInterface::TextureData {
		unique<texture> image;
		unique<texture_view> view;
		dim2<u32> size{};
	};


	error init_rml(span<string_view> args) {
	if (!Rml::Initialise()) {
		return error(generic_errc::unknown, "Rml::Initialise failed");
	}

	return error::no_error;
	}

	void exit_rml() {
		Rml::Shutdown();
	}

	RmlRenderInterface::RmlRenderInterface() {
		upload_queue = Queue::Query(QueueCapability::Graphics);
		program = unique(GraphicsProgram::Create());
		GraphicsProgram::VertexShader(program, std::strlen(kUiVertexShader), kUiVertexShader);
		GraphicsProgram::FragmentShader(program, std::strlen(kUiFragmentShader), kUiFragmentShader);
		vertex_attribute attributes[] = {
			{ 0, static_cast<u32>(offsetof(UiVertex, position)), Format::Rg32Float },
			{ 1, static_cast<u32>(offsetof(UiVertex, uv)), Format::Rg32Float },
			{ 2, static_cast<u32>(offsetof(UiVertex, color)), Format::Rgba32Float },
		};
		vertex_layout layout{ sizeof(UiVertex), attributes, 3 };
		GraphicsProgram::VertexLayout(program, layout);
		GraphicsProgram::RasterState(program, CullMode::None, FrontFace::CounterClockwise, FillMode::Solid);
		GraphicsProgram::BlendState(program, true, RT_BLEND_SRC_ALPHA, RT_BLEND_ONE_MINUS_SRC_ALPHA,
									RT_BLEND_OP_ADD, RT_BLEND_ONE, RT_BLEND_ONE_MINUS_SRC_ALPHA,
									RT_BLEND_OP_ADD);
		GraphicsProgram::Link(program);
		uniform_location = GraphicsProgram::UniformLocation(program, "UiDraw");
		texture_location = GraphicsProgram::UniformLocation(program, "UiTexture");
		create_white_texture();
	}
	RmlRenderInterface::~RmlRenderInterface() {
		collect_garbage();
		delete white_texture;
	}
	void RmlRenderInterface::begin(view<command_buffer> command_buffer, dim2<u32> framebuffer_size) {
		current_command_buffer = command_buffer;
		current_framebuffer_size = framebuffer_size;
		uniform_buffer_index = 0;
	}
	void RmlRenderInterface::end() {
		current_command_buffer = {};
	}
	Rml::CompiledGeometryHandle RmlRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
		auto* geometry = new Geometry;
		vector<UiVertex> flattened;
		flattened.reserve(indices.size());
		for (int index : indices) {
			const Rml::Vertex& source = vertices[static_cast<size_t>(index)];
			UiVertex vertex;
			vertex.position = { source.position.x, source.position.y };
			vertex.uv = { source.tex_coord.x, source.tex_coord.y };
			vertex.color[0] = static_cast<f32>(source.colour.red) / 255.0f;
			vertex.color[1] = static_cast<f32>(source.colour.green) / 255.0f;
			vertex.color[2] = static_cast<f32>(source.colour.blue) / 255.0f;
			vertex.color[3] = static_cast<f32>(source.colour.alpha) / 255.0f;
			flattened.push_back(vertex);
		}
		geometry->vertex_count = static_cast<u32>(flattened.size());
		geometry->vertices = unique(Buffer::Create());
		Buffer::Data(geometry->vertices, BufferMode::Dynamic, BufferUsage::Vertex,
					 static_cast<u64>(flattened.size() * sizeof(UiVertex)), flattened.data());
		return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry);
	}
	void RmlRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) {
		draw_geometry(reinterpret_cast<Geometry*>(geometry), { translation.x, translation.y }, texture);
	}
	void RmlRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
		if (geometry) {
			retired_geometry.push_back(reinterpret_cast<Geometry*>(geometry));
		}
	}
	Rml::TextureHandle RmlRenderInterface::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) {
		int width = 0;
		int height = 0;
		int components = 0;
		std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
			stbi_load(source.c_str(), &width, &height, &components, 4), stbi_image_free);
		if (!pixels || width <= 0 || height <= 0) {
			return 0;
		}
		texture_dimensions = { width, height };
		return create_texture(static_cast<u32>(width), static_cast<u32>(height), pixels.get());
	}
	Rml::TextureHandle RmlRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) {
		return create_texture(static_cast<u32>(source_dimensions.x), static_cast<u32>(source_dimensions.y), source.data());
	}
	void RmlRenderInterface::ReleaseTexture(Rml::TextureHandle texture) {
		if (texture) {
			retired_textures.push_back(reinterpret_cast<TextureData*>(texture));
		}
	}
	void RmlRenderInterface::EnableScissorRegion(bool enable) {
		scissor_enabled = enable;
	}
	void RmlRenderInterface::SetScissorRegion(Rml::Rectanglei region) {
		scissor = region;
	}
	Rml::TextureHandle RmlRenderInterface::create_texture(u32 width, u32 height, const void* pixels) {
		auto* texture_data = new TextureData;
		texture_data->size = { width, height };
		texture_data->image = unique(Texture::Create());
		Queue::Wait(upload_queue, Texture::Data(upload_queue, texture_data->image, RT_TEXTURE_2D, 0,
												width, height, 1, RT_RGBA8_UNORM, pixels));
		texture_data->view = unique(TextureView::CreateFromTexture(texture_data->image));
		TextureView::Filter(texture_data->view, RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_MIP_FILTER_NONE);
		TextureView::Address(texture_data->view, RT_ADDRESS_CLAMP, RT_ADDRESS_CLAMP, RT_ADDRESS_CLAMP);
		return reinterpret_cast<Rml::TextureHandle>(texture_data);
	}
	void RmlRenderInterface::create_white_texture() {
		u08 pixel[] = { 255, 255, 255, 255 };
		white_texture = reinterpret_cast<TextureData*>(create_texture(1, 1, pixel));
	}
	void RmlRenderInterface::draw_geometry(Geometry* geometry, pos2<f32> translation, Rml::TextureHandle texture) {
		if (!geometry || geometry->vertex_count == 0 || !current_command_buffer) {
			return;
		}
		TextureData* texture_data = texture ? reinterpret_cast<TextureData*>(texture) : white_texture;
		UiUniform uniform;
		uniform.viewport_size[0] = static_cast<f32>(std::max(1u, current_framebuffer_size.width));
		uniform.viewport_size[1] = static_cast<f32>(std::max(1u, current_framebuffer_size.height));
		uniform.translation[0] = translation.x;
		uniform.translation[1] = translation.y;
		uniform.texture_mode = texture ? 1.0f : 0.0f;
		if (uniform_buffer_index >= uniform_buffers.size()) {
			uniform_buffers.push_back(unique(Buffer::Create()));
			Buffer::Data(uniform_buffers.back(), BufferMode::Dynamic, BufferUsage::Uniform, sizeof(uniform), &uniform);
		} else {
			Buffer::Subdata(uniform_buffers[uniform_buffer_index], 0, sizeof(uniform), &uniform);
		}
		view<buffer> draw_uniform_buffer = uniform_buffers[uniform_buffer_index];
		++uniform_buffer_index;

		CommandBuffer::UseGraphicsProgram(current_command_buffer, program);
		apply_scissor();
		CommandBuffer::UniformBuffer(current_command_buffer, uniform_location, draw_uniform_buffer, 0, sizeof(uniform));
		CommandBuffer::UniformTexture(current_command_buffer, texture_location, texture_data->view);
		CommandBuffer::BindVertexBuffer(current_command_buffer, geometry->vertices, 0);
		CommandBuffer::Draw(current_command_buffer, geometry->vertex_count, 0);
	}
	void RmlRenderInterface::apply_scissor() {
		const int max_x = static_cast<int>(current_framebuffer_size.width);
		const int max_y = static_cast<int>(current_framebuffer_size.height);
		int left = 0;
		int top = 0;
		int right = max_x;
		int bottom = max_y;
		if (scissor_enabled && scissor.Valid()) {
			left = std::clamp(scissor.Left(), 0, max_x);
			top = std::clamp(scissor.Top(), 0, max_y);
			right = std::clamp(scissor.Right(), left, max_x);
			bottom = std::clamp(scissor.Bottom(), top, max_y);
		}
		CommandBuffer::SetScissor(current_command_buffer, static_cast<u32>(left), static_cast<u32>(top),
								  static_cast<u32>(right - left), static_cast<u32>(bottom - top));
	}
	void RmlRenderInterface::collect_garbage() {
		for (Geometry* geometry : retired_geometry) {
			delete geometry;
		}
		retired_geometry.clear();
		for (TextureData* texture : retired_textures) {
			delete texture;
		}
		retired_textures.clear();
	}

} // namespace lf
