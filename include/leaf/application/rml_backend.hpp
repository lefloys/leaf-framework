#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/filesystem.hpp"
#include "leaf/core/memory.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/graphics/buffer.hpp"
#include "leaf/graphics/command_buffer.hpp"
#include "leaf/graphics/framebuffer.hpp"
#include "leaf/graphics/graphics_program.hpp"
#include "leaf/graphics/queue.hpp"
#include "leaf/graphics/resource.hpp"
#include "leaf/graphics/texture.hpp"
#include "leaf/graphics/texture_view.hpp"
#include "leaf/graphics/uniform_location.hpp"
#include "leaf/core/math/dim.hpp"
#include "leaf/core/math/pos.hpp"

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/ElementInstancer.h>

#include <functional>

namespace lf {
	// A concrete application element supplied before startup. Leaf takes
	// ownership and registers it only after RmlUi itself is initialized.
	struct RmlElementRegistration {
		string tag;
		unique_ptr<Rml::ElementInstancer> instancer;
	};

	class RmlRenderInterface : public Rml::RenderInterface {
	  public:
		struct CustomDrawContext {
			rt::view<rt::command_buffer> commands;
			dim2<u32> viewport_size{};
			int left = 0;
			int top = 0;
			int right = 0;
			int bottom = 0;
		};
		using CustomDraw = std::function<void(const CustomDrawContext&)>;

		RmlRenderInterface();
		~RmlRenderInterface() override;

		void begin(rt::view<rt::command_buffer> command_buffer, dim2<u32> viewport_size);
		void end();
		void flush_released_resources();
		void QueueCustomDraw(CustomDraw draw);

		Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
		void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
		void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;
		Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
		Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
		void ReleaseTexture(Rml::TextureHandle texture) override;
		void EnableScissorRegion(bool enable) override;
		void SetScissorRegion(Rml::Rectanglei region) override;

	  private:
		struct UiVertex {
			pos2<f32> position;
			pos2<f32> uv;
			f32 color[4];
		};

		struct Geometry;
		struct TextureData;
		struct QueuedGeometry;

		unique_ptr<TextureData> create_texture_data(u32 width, u32 height, const void* pixels);
		Rml::TextureHandle create_texture(u32 width, u32 height, const void* pixels);
		void draw_geometry(Geometry* geometry, pos2<f32> translation, Rml::TextureHandle texture);
		void flush_queued_geometry();
		void draw_batch(const vector<UiVertex>& vertices, TextureData* texture, int left, int top, int right, int bottom);
		void collect_garbage();

		rt::handle<rt::queue> upload_queue;
		rt::view<rt::command_buffer> current_command_buffer;
		dim2<u32> current_framebuffer_size{};
		rt::unique<rt::graphics_program> program;
		vector<rt::unique<rt::buffer>> uniform_buffers;
		usize uniform_buffer_index = 0;
		usize batch_vertex_buffer_index = 0;
		rt::uniform_location uniform_location{};
		rt::uniform_location texture_location{};
		unique_ptr<TextureData> white_texture;
		bool scissor_enabled = false;
		bool program_bound = false;
		Rml::Rectanglei scissor{};
		rt_texture_view bound_texture = RT_NULL_HANDLE;
		vector<QueuedGeometry> queued_geometry;
		vector<UiVertex> batch_vertices;
		vector<rt::unique<rt::buffer>> batch_vertex_buffers;
		vector<u64> batch_vertex_buffer_sizes;
		vector<unique_ptr<Geometry>> retired_geometry;
		vector<unique_ptr<TextureData>> retired_textures;
	};

	error init_rml(span<string_view> args, vector<RmlElementRegistration> elements = {});
	void exit_rml();
	RmlRenderInterface& rml_renderer();
} // namespace lf
