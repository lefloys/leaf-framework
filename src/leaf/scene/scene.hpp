#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/filesystem.hpp>
#include <leaf/core/string.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/graphics/command_buffer.hpp>
#include <leaf/graphics/framebuffer.hpp>
#include <leaf/graphics/resource.hpp>
#include <leaf/graphics/window.hpp>
#include <leaf/math/dim.hpp>

#include <RmlUi/Core/EventListener.h>
#include <sol/sol.hpp>

#include <functional>
#include <memory>

namespace Rml {
	class Context;
	class ElementDocument;
	class ElementInstancer;
}

namespace lf {
	class RmlRenderInterface;

	class Scene {
	  public:
		Scene();
		~Scene();

		error load(const fs::path& rml_path);
		error load_memory(string_view rml, string_view source_name, string_view lua_source);
		void set_lua_binder(std::function<void(sol::state&)> binder);
		void set_rml_binder(std::function<void()> binder);
		void process_input(view<window> window);
		void update(f64 delta_seconds);
		void render(view<command_buffer> command_buffer, view<framebuffer> framebuffer, dim2<u32> framebuffer_size);
		void set_text(string_view element_id, string_view text);
		void set_position(string_view element_id, f32 x, f32 y);
		void set_progress(string_view element_id, f32 progress);
		void shutdown();

	  private:
		error load_script(const fs::path& rml_path);
		error load_script_source(string_view source, string_view source_name);
		void initialize_rml();
		void initialize_lua();
		error finish_loaded_document(string_view lua_source, string_view lua_source_name);
		void bind_lua_api();
		void bind_script_events();
		void add_lua_event_listener(Rml::Element* element, string_view event_name, string_view attribute_name);
		void invoke_script_function(string_view function_name);
		void invoke_script_function_if_present(string_view function_name, f64 argument = 0.0);
		void set_loading_bar_progress(string_view element_id, f32 progress);

		std::function<void(sol::state&)> lua_binder;
		std::function<void()> rml_binder;
		sol::state lua;
		vector<std::unique_ptr<Rml::EventListener>> event_listeners;
		Rml::Context* context = nullptr;
		Rml::ElementDocument* document = nullptr;
	};
} // namespace lf
