#include "leaf/application/rml_backend.hpp"

#include "scene_systems.hpp"

#include "embed/font.h"
#include "leaf/core/exception.hpp"
#include "leaf/core/format.hpp"
#include "leaf/core/array.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/graphics/timepoint.hpp"
#include "leaf/platform/platform.hpp"
#include "leaf/script/virtual_filesystem.hpp"

#include <RmlUi/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/SystemInterface.h>
#include <rtsl/program.hpp>
#include <stb_image.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <memory>

extern "C" const rtsl::ProgramBytes leaf_rml_ui_rtslp;

namespace lf {
	class RmlSystemInterface : public Rml::SystemInterface {
	  public:
		bool LogMessage(Rml::Log::Type type, const Rml::String& message) override {
			switch (type) {
			case Rml::Log::LT_ERROR:
			case Rml::Log::LT_ASSERT:
				log::Error("[rml] {}", message);
				break;
			case Rml::Log::LT_WARNING:
				log::Warning("[rml] {}", message);
				break;
			case Rml::Log::LT_DEBUG:
				log::Debug("[rml] {}", message);
				break;
			default:
				log::Info("[rml] {}", message);
				break;
			}
			return true;
		}

		void JoinPath(Rml::String& translated_path, const Rml::String& document_path, const Rml::String& path) override {
			if (IsVirtualPath(string(path))) {
				translated_path = path;
				return;
			}
			Rml::SystemInterface::JoinPath(translated_path, document_path, path);
		}

		void SetClipboardText(const Rml::String& text) override {
			platform_clipboard_text(text);
		}

		void GetClipboardText(Rml::String& text) override {
			text = platform_clipboard_text();
		}
	};

	class RmlFileInterface final : public Rml::FileInterface {
	  public:
		Rml::FileHandle Open(const Rml::String& path) override {
			fs::path resource_path;
			if (IsVirtualPath(string(path))) {
				report<fs::path> resolved = ResolveVirtualPathReport(string(path));
				if (!resolved) {
					log::Warning("[rml] failed to resolve resource '{}': {}", path, resolved.error().message);
					return 0;
				}
				resource_path = std::move(*resolved);
			} else {
				resource_path = fs::folder::install / string(path);
			}
			std::unique_ptr<std::ifstream> stream = std::make_unique<std::ifstream>(resource_path, std::ios::binary);
			if (!*stream) {
				log::Warning("[rml] failed to open resource '{}'", resource_path.string());
				return 0;
			}
			return reinterpret_cast<Rml::FileHandle>(stream.release());
		}

		void Close(Rml::FileHandle file) override {
			delete reinterpret_cast<std::ifstream*>(file);
		}

		size_t Read(void* buffer, size_t size, Rml::FileHandle file) override {
			std::ifstream* stream = reinterpret_cast<std::ifstream*>(file);
			if (!stream || !buffer || size == 0) {
				return 0;
			}
			stream->read(static_cast<char*>(buffer), static_cast<std::streamsize>(size));
			return static_cast<size_t>(stream->gcount());
		}

		bool Seek(Rml::FileHandle file, long offset, int origin) override {
			std::ifstream* stream = reinterpret_cast<std::ifstream*>(file);
			if (!stream) {
				return false;
			}
			std::ios::seekdir direction;
			switch (origin) {
			case SEEK_SET: direction = std::ios::beg; break;
			case SEEK_CUR: direction = std::ios::cur; break;
			case SEEK_END: direction = std::ios::end; break;
			default: return false;
			}
			stream->clear();
			stream->seekg(offset, direction);
			return static_cast<bool>(*stream);
		}

		size_t Tell(Rml::FileHandle file) override {
			std::ifstream* stream = reinterpret_cast<std::ifstream*>(file);
			if (!stream) {
				return 0;
			}
			const std::streampos position = stream->tellg();
			return position >= 0 ? static_cast<size_t>(position) : 0;
		}
	};

	struct RmlBackendState {
		std::unique_ptr<RmlRenderInterface> renderer;
		std::unique_ptr<RmlSystemInterface> system;
		std::unique_ptr<RmlFileInterface> file;
		unique_ptr<Rml::ElementInstancer> document_instancer;
		vector<RmlElementRegistration> application_elements;
	};

	static RmlBackendState rml_backend;

	struct UiUniform {
		f32 viewport_size[2] = { 1.0f, 1.0f };
		f32 translation[2] = { 0.0f, 0.0f };
		f32 texture_mode = 0.0f;
		f32 padding = 0.0f;
	};

	struct RmlRenderInterface::Geometry {
		vector<UiVertex> vertices;
		u32 vertex_count = 0;
	};

	struct RmlRenderInterface::TextureData {
		rt::unique<rt::texture> image;
		rt::unique<rt::texture_view> view;
		rt::unique<rt::sampler> sampler;
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

	error init_rml(span<string_view> args, vector<RmlElementRegistration> elements) {
		if (rml_backend.renderer || rml_backend.system || rml_backend.file) {
			return error(generic_errc::unknown, "RmlUi is already initialized");
		}
		(void)args;
		log::Debug("[leaf] Starting interface...");
		std::unique_ptr<RmlSystemInterface> system = std::make_unique<RmlSystemInterface>();
		std::unique_ptr<RmlRenderInterface> renderer = std::make_unique<RmlRenderInterface>();
		std::unique_ptr<RmlFileInterface> file = std::make_unique<RmlFileInterface>();
		Rml::SetSystemInterface(system.get());
		Rml::SetRenderInterface(renderer.get());
		Rml::SetFileInterface(file.get());

		if (!Rml::Initialise()) {
			Rml::SetRenderInterface(nullptr);
			Rml::SetSystemInterface(nullptr);
			Rml::SetFileInterface(nullptr);
			return error(generic_errc::unknown, "Rml::Initialise failed");
		}
		auto document_instancer = make_unique<Rml::ElementInstancerGeneric<SceneDocument>>();
		Rml::Factory::RegisterElementInstancer("body", document_instancer.get());
		for (const RmlElementRegistration& element : elements) {
			if (element.tag.empty() || !element.instancer) {
				Rml::Shutdown();
				Rml::SetRenderInterface(nullptr);
				Rml::SetSystemInterface(nullptr);
				Rml::SetFileInterface(nullptr);
				return error(generic_errc::input_error, "Rml element registrations require a tag and instancer");
			}
			Rml::Factory::RegisterElementInstancer(Rml::String(element.tag), element.instancer.get());
		}
		if (!Rml::LoadFontFace({ reinterpret_cast<const Rml::byte*>(Comic_Sans_MS_ttf), sizeof(Comic_Sans_MS_ttf) }, "Comic Sans MS", Rml::Style::FontStyle::Normal, Rml::Style::FontWeight::Auto, true)) {
			Rml::Shutdown();
			Rml::SetRenderInterface(nullptr);
			Rml::SetSystemInterface(nullptr);
			Rml::SetFileInterface(nullptr);
			return error(generic_errc::unknown, "failed to load default RmlUi font");
		}
		rml_backend.system = std::move(system);
		rml_backend.renderer = std::move(renderer);
		rml_backend.file = std::move(file);
		rml_backend.document_instancer = std::move(document_instancer);
		rml_backend.application_elements = std::move(elements);

		return error::no_error;
	}

	void exit_rml() {
		if (!rml_backend.renderer) {
			return;
		}
		Rml::ReleaseRenderManagers();
		rml_backend.renderer->flush_released_resources();
		Rml::Shutdown();
		rml_backend.document_instancer.reset();
		rml_backend.application_elements.clear();
		Rml::SetRenderInterface(nullptr);
		Rml::SetSystemInterface(nullptr);
		Rml::SetFileInterface(nullptr);
		rml_backend.renderer.reset();
		rml_backend.system.reset();
		rml_backend.file.reset();
	}

	RmlRenderInterface& rml_renderer() {
		if (!rml_backend.renderer) {
			throw runtime_exception("Rml render interface requested before RmlUi initialization");
		}
		return *rml_backend.renderer;
	}

	RmlRenderInterface::RmlRenderInterface() {
		upload_queue = rt::unique(rt::Queue::Create(rt::QueueCapability::Graphics));
		upload_commands = rt::unique(rt::Cmd::Create());
		program = rt::unique(rt::Program::Create());
		rt::Program::Source(program, "main", leaf_rml_ui_rtslp.view());
		rt::vertex_attribute attributes[] = {
			{ "position", static_cast<u32>(offsetof(UiVertex, position)), rt::Format::Rg32Float },
			{ "uv", static_cast<u32>(offsetof(UiVertex, uv)), rt::Format::Rg32Float },
			{ "color", static_cast<u32>(offsetof(UiVertex, color)), rt::Format::Rgba32Float },
		};
		rt::vertex_input input{ attributes, 3, sizeof(UiVertex) };
		rt::vertex_layout layout{ &input, 1 };
		rt::Program::VertexLayout(program, layout);
		rt::Program::RasterState(program, rt::CullMode::None, rt::FrontFace::CounterClockwise, rt::FillMode::Solid);
		rt::Program::BlendState(program, true, RT_BLEND_ONE, RT_BLEND_ONE_MINUS_SRC_ALPHA, RT_BLEND_OP_ADD, RT_BLEND_ONE, RT_BLEND_ONE_MINUS_SRC_ALPHA, RT_BLEND_OP_ADD);
		rt::Program::Finalize(program);
		vertex_location = rt::Program::InputLocation(program, { attributes, 3 });
		uniform_location = rt::Program::UniformLocation(program, "UiDraw");
		texture_location = rt::Program::UniformLocation(program, "UiTexture");
		const array<u08, 4> white_pixel{ 255, 255, 255, 255 };
		white_texture = create_texture_data(1, 1, white_pixel.data());
	}
	RmlRenderInterface::~RmlRenderInterface() {
		collect_garbage();
	}
	void RmlRenderInterface::begin(rt::view<rt::command_buffer> command_buffer, dim2<u32> viewport_size) {
		current_command_buffer = command_buffer;
		current_framebuffer_size = viewport_size;
		batch_vertex_buffer_index = 0;
		program_bound = false;
		bound_texture = RT_NULL_HANDLE;
		queued_geometry.clear();
	}
	void RmlRenderInterface::end() {
		flush_queued_geometry();
		current_command_buffer = {};
		// Reclaim retired geometries/textures now that the frame's flush has
		// copied any vertex data the GPU needs into batch_vertex_buffers.
		// Without this, ReleaseGeometry-pushed entries pile up forever (the
		// only other callers of collect_garbage are shutdown paths) and the
		// per-frame set_rml churn on overlays leaks ~1 MB/s of UiVertex[].
		collect_garbage();
	}
	void RmlRenderInterface::flush_released_resources() {
		collect_garbage();
	}
	void RmlRenderInterface::QueueCustomDraw(CustomDraw draw) {
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
		rt::detail::check_rutile_error("failed to record custom RmlUi draw");
	}
	Rml::CompiledGeometryHandle RmlRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
		unique_ptr<Geometry> geometry = make_unique<Geometry>();
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
		return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry.release());
	}
	void RmlRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) {
		draw_geometry(reinterpret_cast<Geometry*>(geometry), { translation.x, translation.y }, texture);
	}
	void RmlRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
		if (geometry) {
			retired_geometry.emplace_back(reinterpret_cast<Geometry*>(geometry));
		}
	}
	Rml::TextureHandle RmlRenderInterface::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) {
		fs::path path;
		try {
			path = ResolveVirtualPath(source.c_str());
		} catch (const lf::exception& e) {
			log::Warning("{}", lf::format("[rml] failed to resolve texture '{}': {}", source, e.what()));
			return 0;
		}

		int width = 0;
		int height = 0;
		int components = 0;
		std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
			stbi_load(path.string().c_str(), &width, &height, &components, 4), stbi_image_free
		);
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
			retired_textures.emplace_back(reinterpret_cast<TextureData*>(texture));
		}
	}
	void RmlRenderInterface::EnableScissorRegion(bool enable) {
		scissor_enabled = enable;
	}
	void RmlRenderInterface::SetScissorRegion(Rml::Rectanglei region) {
		scissor = region;
	}
	unique_ptr<RmlRenderInterface::TextureData> RmlRenderInterface::create_texture_data(u32 width, u32 height, const void* pixels) {
		unique_ptr<TextureData> texture_data = make_unique<TextureData>();
		texture_data->size = { width, height };
		texture_data->image = rt::unique<rt::texture>(rt::Texture::Create());
		rt::Texture::Resize(texture_data->image, RT_TEXTURE_2D, RT_RGBA8_UNORM, { width, height, 1 });
		rt::Cmd::Reset(upload_commands);
		rt::Cmd::Begin(upload_commands);
		rt::Cmd::TextureData(upload_commands, texture_data->image, { RT_TEXTURE_ASPECT_COLOR, 0, 1, 0, 1, { width, height, 1 }, {} }, reinterpret_cast<const u08*>(pixels));
		rt::Cmd::End(upload_commands);
		rt::Timepoint::Wait(rt::Queue::Submit(upload_queue, upload_commands));
		texture_data->view = rt::unique<rt::texture_view>(rt::TextureView::CreateFromTexture(texture_data->image));
		texture_data->sampler = rt::unique<rt::sampler>(rt::Sampler::Create());
		rt::Sampler::SetFilter(texture_data->sampler, RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_MIP_FILTER_NONE);
		rt::Sampler::SetAddress(texture_data->sampler, RT_ADDRESS_CLAMP, RT_ADDRESS_CLAMP, RT_ADDRESS_CLAMP);
		return texture_data;
	}
	Rml::TextureHandle RmlRenderInterface::create_texture(u32 width, u32 height, const void* pixels) {
		return reinterpret_cast<Rml::TextureHandle>(create_texture_data(width, height, pixels).release());
	}
	void RmlRenderInterface::draw_geometry(Geometry* geometry, pos2<f32> translation, Rml::TextureHandle texture) {
		if (!geometry || geometry->vertex_count == 0 || !current_command_buffer) {
			return;
		}
		TextureData* texture_data = texture ? reinterpret_cast<TextureData*>(texture) : white_texture.get();
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
		if (vertices.empty() || !texture_data) {
			return;
		}

		const u64 vertex_bytes = static_cast<u64>(vertices.size() * sizeof(UiVertex));
		if (batch_vertex_buffer_index >= batch_vertex_buffers.size()) {
			batch_vertex_buffers.push_back(rt::unique<rt::buffer>(rt::Buffer::Create()));
			batch_vertex_buffer_sizes.push_back(vertex_bytes);
			rt::Buffer::Resize(batch_vertex_buffers.back(), RT_DEVICE_MEMORY, vertex_bytes);
		} else if (batch_vertex_buffer_sizes[batch_vertex_buffer_index] < vertex_bytes) {
			batch_vertex_buffer_sizes[batch_vertex_buffer_index] = vertex_bytes;
			rt::Buffer::Resize(batch_vertex_buffers[batch_vertex_buffer_index], RT_DEVICE_MEMORY, vertex_bytes);
		}
		rt::view<rt::buffer> draw_vertices = batch_vertex_buffers[batch_vertex_buffer_index];
		++batch_vertex_buffer_index;

		UiUniform uniform;
		uniform.viewport_size[0] = static_cast<f32>(std::max(1u, current_framebuffer_size.width));
		uniform.viewport_size[1] = static_cast<f32>(std::max(1u, current_framebuffer_size.height));
		uniform.translation[0] = 0.0f;
		uniform.translation[1] = 0.0f;
		uniform.texture_mode = texture_data == white_texture.get() ? 0.0f : 1.0f;
		{
			rt::view<rt::program> draw_program = program;
			rt::view<rt::texture_view> draw_texture = texture_data->view;
			if (!program_bound) {
				rt::Cmd::UseProgram(current_command_buffer, draw_program);
				program_bound = true;
			}
			rt::Cmd::BufferData(current_command_buffer, draw_vertices, { vertex_bytes, 0 }, reinterpret_cast<const u08*>(vertices.data()));
			rt::Cmd::SetScissor(current_command_buffer, static_cast<u32>(left), static_cast<u32>(top), static_cast<u32>(right - left), static_cast<u32>(bottom - top));
			rt::Cmd::UniformData(current_command_buffer, uniform_location, reinterpret_cast<const u08*>(&uniform), sizeof(uniform));
			if (bound_texture != draw_texture.value) {
				rt::Cmd::BindTexture(current_command_buffer, texture_location, draw_texture);
				rt::Cmd::BindSampler(current_command_buffer, texture_location, texture_data->sampler);
				bound_texture = draw_texture.value;
			}
			rt::Cmd::VertexBuffer(current_command_buffer, vertex_location, draw_vertices, { vertex_bytes, 0 });
			rt::Cmd::Draw(current_command_buffer, static_cast<u32>(vertices.size()), 0);
		}
		rt::detail::check_rutile_error("failed to record RmlUi geometry");
	}
	void RmlRenderInterface::collect_garbage() {
		retired_geometry.clear();
		retired_textures.clear();
	}

} // namespace lf
