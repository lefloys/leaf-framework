#include "rml_backend.hpp"

#include <leaf/core/format.hpp>
#include <leaf/core/messages.hpp>
#include <leaf/core/profiler.hpp>
#include <leaf/platform/platform.hpp>
#include <leaf/graphics/timepoint.hpp>
#include <leaf/script/virtual_filesystem.hpp>

#include <embed/font.h>

#include <RmlUi/Core.h>
#include <RmlUi/Core/SystemInterface.h>
#include <stb_image.h>

#include <algorithm>
#include <cstring>
#include <memory>

namespace lf {
	class RmlSystemInterface : public Rml::SystemInterface {
	  public:
		void SetClipboardText(const Rml::String& text) override {
			platform_clipboard_text(text);
		}

		void GetClipboardText(Rml::String& text) override {
			text = platform_clipboard_text();
		}
	};

	static std::unique_ptr<RmlRenderInterface> g_rml_renderer;
	static std::unique_ptr<RmlSystemInterface> g_rml_system;
	static bool g_rml_initialized = false;

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
		vector<UiVertex> vertices;
		u32 vertex_count = 0;
	};

	struct RmlRenderInterface::TextureData {
		unique<texture> image;
		unique<texture_view> view;
		dim2<u32> size{};
	};

	struct RmlRenderInterface::QueuedGeometry {
		Geometry* geometry = nullptr;
		TextureData* texture = nullptr;
		pos2<f32> translation{};
		int left = 0;
		int top = 0;
		int right = 0;
		int bottom = 0;
	};


	error init_rml(span<string_view> args) {
		g_rml_system = std::make_unique<RmlSystemInterface>();
		g_rml_renderer = std::make_unique<RmlRenderInterface>();
		Rml::SetSystemInterface(g_rml_system.get());
		Rml::SetRenderInterface(g_rml_renderer.get());

		if (!Rml::Initialise()) {
			g_rml_renderer.reset();
			g_rml_system.reset();
			return error(generic_errc::unknown, "Rml::Initialise failed");
		}
		g_rml_initialized = true;

		Rml::Span<const Rml::byte> default_font(
			reinterpret_cast<const Rml::byte*>(Comic_Sans_MS_ttf),
			sizeof(Comic_Sans_MS_ttf));
		if (!Rml::LoadFontFace(default_font, "Comic Sans MS", Rml::Style::FontStyle::Normal,
							   Rml::Style::FontWeight::Auto, true)) {
			Rml::Shutdown();
			g_rml_initialized = false;
			g_rml_renderer.reset();
			g_rml_system.reset();
			return error(generic_errc::unknown, "failed to load default RmlUi font");
		}

		return error::no_error;
	}

	void exit_rml() {
		if (g_rml_initialized) {
			Rml::ReleaseRenderManagers();
			g_rml_renderer->flush_released_resources();
			Rml::Shutdown();
			g_rml_initialized = false;
		}
		if (g_rml_renderer) {
			g_rml_renderer->flush_released_resources();
			g_rml_renderer.reset();
		}
		g_rml_system.reset();
	}

	RmlRenderInterface& rml_renderer() {
		return *g_rml_renderer;
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
		GraphicsProgram::BlendState(program, true, RT_BLEND_ONE, RT_BLEND_ONE_MINUS_SRC_ALPHA,
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
	void RmlRenderInterface::begin(view<command_buffer> command_buffer, dim2<u32> viewport_size) {
		LF_PROFILE_SCOPE("RmlRenderer::FrameBegin");
		current_command_buffer = command_buffer;
		current_framebuffer_size = viewport_size;
		uniform_buffer_index = 0;
		batch_vertex_buffer_index = 0;
		program_bound = false;
		bound_texture = RT_NULL_HANDLE;
		queued_geometry.clear();
	}
	void RmlRenderInterface::end() {
		LF_PROFILE_SCOPE("RmlRenderer::FrameEnd");
		flush_queued_geometry();
		current_command_buffer = {};
	}
	void RmlRenderInterface::flush_released_resources() {
		LF_PROFILE_SCOPE("RmlRenderer::FlushReleasedResources");
		collect_garbage();
	}
	void RmlRenderInterface::QueueCustomDraw(CustomDraw draw) {
		LF_PROFILE_SCOPE("RmlRenderer::CustomDraw");
		if (!current_command_buffer || !draw) {
			return;
		}

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
		if (right <= left || bottom <= top) {
			return;
		}

		flush_queued_geometry();
		draw(CustomDrawContext{
			.commands = current_command_buffer,
			.viewport_size = current_framebuffer_size,
			.left = left,
			.top = top,
			.right = right,
			.bottom = bottom,
		});
		program_bound = false;
		bound_texture = RT_NULL_HANDLE;
		detail::check_rutile_error("failed to record custom RmlUi draw");
	}
	Rml::CompiledGeometryHandle RmlRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
		LF_PROFILE_SCOPE("RmlRenderer::CompileGeometry");
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
		geometry->vertices = std::move(flattened);
		geometry->vertex_count = static_cast<u32>(geometry->vertices.size());
		return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry);
	}
	void RmlRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) {
		LF_PROFILE_SCOPE("RmlRenderer::RenderGeometry");
		draw_geometry(reinterpret_cast<Geometry*>(geometry), { translation.x, translation.y }, texture);
	}
	void RmlRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
		LF_PROFILE_SCOPE("RmlRenderer::ReleaseGeometry");
		if (geometry) {
			retired_geometry.push_back(reinterpret_cast<Geometry*>(geometry));
		}
	}
	Rml::TextureHandle RmlRenderInterface::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) {
		LF_PROFILE_SCOPE("RmlRenderer::LoadTexture");
		fs::path path;
		try {
			path = ResolveVirtualPath(source.c_str());
		} catch (const lf::exception& e) {
			log_warning(format("[rml] failed to resolve texture '{}': {}", source, e.what()));
			return 0;
		}

		int width = 0;
		int height = 0;
		int components = 0;
		std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
			stbi_load(path.string().c_str(), &width, &height, &components, 4), stbi_image_free);
		if (!pixels || width <= 0 || height <= 0) {
			return 0;
		}
		texture_dimensions = { width, height };
		return create_texture(static_cast<u32>(width), static_cast<u32>(height), pixels.get());
	}
	Rml::TextureHandle RmlRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) {
		LF_PROFILE_SCOPE("RmlRenderer::GenerateTexture");
		return create_texture(static_cast<u32>(source_dimensions.x), static_cast<u32>(source_dimensions.y), source.data());
	}
	void RmlRenderInterface::ReleaseTexture(Rml::TextureHandle texture) {
		LF_PROFILE_SCOPE("RmlRenderer::ReleaseTexture");
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
		LF_PROFILE_SCOPE("RmlRenderer::CreateTexture");
		auto* texture_data = new TextureData;
		texture_data->size = { width, height };
		texture_data->image = unique(Texture::Create());
		Timepoint::Wait(Texture::Data(upload_queue, texture_data->image, RT_TEXTURE_2D, 0,
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
		LF_PROFILE_SCOPE("RmlRenderer::DrawGeometry");
		if (!geometry || geometry->vertex_count == 0 || !current_command_buffer) {
			return;
		}
		TextureData* texture_data = texture ? reinterpret_cast<TextureData*>(texture) : white_texture;
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
		queued_geometry.push_back({
			.geometry = geometry,
			.texture = texture_data,
			.translation = translation,
			.left = left,
			.top = top,
			.right = right,
			.bottom = bottom,
		});
	}
	void RmlRenderInterface::flush_queued_geometry() {
		LF_PROFILE_SCOPE("RmlRenderer::FlushQueuedGeometry");
		if (queued_geometry.empty() || !current_command_buffer) {
			return;
		}

		usize first = 0;
		while (first < queued_geometry.size()) {
			const QueuedGeometry& first_item = queued_geometry[first];
			usize last = first + 1;
			while (last < queued_geometry.size()) {
				const QueuedGeometry& item = queued_geometry[last];
				if (item.texture != first_item.texture ||
					item.left != first_item.left ||
					item.top != first_item.top ||
					item.right != first_item.right ||
					item.bottom != first_item.bottom) {
					break;
				}
				++last;
			}

			batch_vertices.clear();
			u64 vertex_count = 0;
			for (usize index = first; index < last; ++index) {
				vertex_count += queued_geometry[index].geometry->vertices.size();
			}
			batch_vertices.reserve(static_cast<usize>(vertex_count));
			for (usize index = first; index < last; ++index) {
				const QueuedGeometry& item = queued_geometry[index];
				for (UiVertex vertex : item.geometry->vertices) {
					vertex.position.x += item.translation.x;
					vertex.position.y += item.translation.y;
					batch_vertices.push_back(vertex);
				}
			}
			draw_batch(batch_vertices, first_item.texture, first_item.left, first_item.top, first_item.right, first_item.bottom);
			first = last;
		}
		queued_geometry.clear();
	}
	void RmlRenderInterface::draw_batch(const vector<UiVertex>& vertices, TextureData* texture_data, int left, int top, int right, int bottom) {
		LF_PROFILE_SCOPE("RmlRenderer::DrawBatch");
		if (vertices.empty() || !texture_data) {
			return;
		}

		const u64 vertex_bytes = static_cast<u64>(vertices.size() * sizeof(UiVertex));
		if (batch_vertex_buffer_index >= batch_vertex_buffers.size()) {
			LF_PROFILE_SCOPE("RmlRenderer::CreateBatchVertexBuffer");
			batch_vertex_buffers.push_back(unique(Buffer::Create()));
			batch_vertex_buffer_sizes.push_back(vertex_bytes);
			Buffer::Data(batch_vertex_buffers.back(), BufferMode::Dynamic, BufferUsage::Vertex, vertex_bytes, vertices.data());
		} else if (batch_vertex_buffer_sizes[batch_vertex_buffer_index] < vertex_bytes) {
			LF_PROFILE_SCOPE("RmlRenderer::ResizeBatchVertexBuffer");
			batch_vertex_buffer_sizes[batch_vertex_buffer_index] = vertex_bytes;
			Buffer::Data(batch_vertex_buffers[batch_vertex_buffer_index], BufferMode::Dynamic, BufferUsage::Vertex, vertex_bytes, vertices.data());
		} else {
			LF_PROFILE_SCOPE("RmlRenderer::UpdateBatchVertexBuffer");
			Buffer::Subdata(batch_vertex_buffers[batch_vertex_buffer_index], 0, vertex_bytes, vertices.data());
		}
		view<buffer> draw_vertices = batch_vertex_buffers[batch_vertex_buffer_index];
		++batch_vertex_buffer_index;

		UiUniform uniform;
		uniform.viewport_size[0] = static_cast<f32>(std::max(1u, current_framebuffer_size.width));
		uniform.viewport_size[1] = static_cast<f32>(std::max(1u, current_framebuffer_size.height));
		uniform.translation[0] = 0.0f;
		uniform.translation[1] = 0.0f;
		uniform.texture_mode = texture_data == white_texture ? 0.0f : 1.0f;
		if (uniform_buffer_index >= uniform_buffers.size()) {
			LF_PROFILE_SCOPE("RmlRenderer::CreateUniformBuffer");
			uniform_buffers.push_back(unique(Buffer::Create()));
			Buffer::Data(uniform_buffers.back(), BufferMode::Dynamic, BufferUsage::Uniform, sizeof(uniform), &uniform);
		} else {
			LF_PROFILE_SCOPE("RmlRenderer::UpdateUniformBuffer");
			Buffer::Subdata(uniform_buffers[uniform_buffer_index], 0, sizeof(uniform), &uniform);
		}
		view<buffer> draw_uniform_buffer = uniform_buffers[uniform_buffer_index];
		++uniform_buffer_index;

		{
			LF_PROFILE_SCOPE("RmlRenderer::RecordDrawCommands");
			view<graphics_program> draw_program = program;
			view<texture_view> draw_texture = texture_data->view;
			if (!program_bound) {
				rtCmdUseGraphicsProgram(current_command_buffer, draw_program);
				program_bound = true;
			}
			rtCmdSetScissor(current_command_buffer, static_cast<u32>(left), static_cast<u32>(top),
							static_cast<u32>(right - left), static_cast<u32>(bottom - top));
			rtCmdUniformBuffer(current_command_buffer, uniform_location, draw_uniform_buffer, 0, sizeof(uniform));
			if (bound_texture != draw_texture.value) {
				rtCmdUniformTexture(current_command_buffer, texture_location, draw_texture);
				bound_texture = draw_texture.value;
			}
			rtCmdBindVertexBuffer(current_command_buffer, draw_vertices, 0);
			rtCmdDraw(current_command_buffer, static_cast<u32>(vertices.size()), 0);
		}
		detail::check_rutile_error("failed to record RmlUi geometry");
	}
	void RmlRenderInterface::collect_garbage() {
		LF_PROFILE_SCOPE("RmlRenderer::CollectGarbage");
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
