#pragma once

#include <leaf/core/filesystem.hpp>
#include <leaf/core/error.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/graphics/buffer.hpp>
#include <leaf/graphics/command_buffer.hpp>
#include <leaf/graphics/framebuffer.hpp>
#include <leaf/graphics/graphics_program.hpp>
#include <leaf/graphics/queue.hpp>
#include <leaf/graphics/resource.hpp>
#include <leaf/graphics/texture.hpp>
#include <leaf/graphics/texture_view.hpp>
#include <leaf/graphics/uniform_location.hpp>
#include <leaf/math/dim.hpp>
#include <leaf/math/pos.hpp>

#include <RmlUi/Core/RenderInterface.h>

namespace lf {
	class RmlRenderInterface : public Rml::RenderInterface {
	  public:
		RmlRenderInterface();
		~RmlRenderInterface() override;

		void begin(view<command_buffer> command_buffer, dim2<u32> framebuffer_size);
		void end();

		Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
													Rml::Span<const int> indices) override;
		void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation,
							Rml::TextureHandle texture) override;
		void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;
		Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions,
									   const Rml::String& source) override;
		Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source,
										   Rml::Vector2i source_dimensions) override;
		void ReleaseTexture(Rml::TextureHandle texture) override;
		void EnableScissorRegion(bool enable) override;
		void SetScissorRegion(Rml::Rectanglei region) override;

	  private:
		struct Geometry;
		struct TextureData;

		Rml::TextureHandle create_texture(u32 width, u32 height, const void* pixels);
		void create_white_texture();
		void draw_geometry(Geometry* geometry, pos2<f32> translation, Rml::TextureHandle texture);
		void apply_scissor();
		void collect_garbage();

		handle<queue> upload_queue;
		view<command_buffer> current_command_buffer;
		dim2<u32> current_framebuffer_size{};
		unique<graphics_program> program;
		vector<unique<buffer>> uniform_buffers;
		usize uniform_buffer_index = 0;
		lf::uniform_location uniform_location{};
		lf::uniform_location texture_location{};
		TextureData* white_texture = nullptr;
		bool scissor_enabled = false;
		Rml::Rectanglei scissor{};
		vector<Geometry*> retired_geometry;
		vector<TextureData*> retired_textures;
	};

	error initialize_rml_runtime();
	void shutdown_rml_runtime();
	RmlRenderInterface& rml_render_interface();
} // namespace lf
