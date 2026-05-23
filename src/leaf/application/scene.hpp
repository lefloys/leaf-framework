#pragma once

#include <leaf/core/memory.hpp>
#include <leaf/core/string.hpp>
#include <leaf/graphics/window.hpp>

#include <RmlUi/Core/Types.h>
#include <sol/sol.hpp>

#include <functional>
#include <chrono>
#include <memory>
#include <vector>

namespace Rml {
	class Context;
	class Element;
	class ElementDocument;
	class Event;
}

namespace lf {
	struct input_event;

	class Scene {
	public:
		Scene(Rml::Context& context, view<window> display, string_view initial);
		~Scene();

		void update();
		void input(const input_event& event);
		bool accepts_text_input(u32 character) const;
		string filter_text_input(string_view text) const;
		void fixed_update();
		void render();
		void set_rml(string_view id, string_view rml);
		void set_attribute(string_view id, string_view name, string_view value);
		void set_attribute(string_view id, string_view name, f32 value);
		void queue_script(string script, string source_name);
		void refresh_title();
		string_view title() const;

		using ScriptInstaller = std::function<void(sol::state&, Rml::ElementDocument&)>;
		static void RegisterScriptInstaller(ScriptInstaller installer);

	private:
		class ScriptEventListener;

		void bind_script_events();
		void clear_script_events();
		void run_inline_scripts(string_view source);
		void run_script(string_view script, string_view source_name);
		void install_ui_automation_helpers();
		void update_ui_automation(f64 elapsed);
		Rml::Element* find_element(string_view id) const;
		Rml::Element* find_selector(string_view selector, i32 index) const;
		bool run_element_script_event(Rml::Element* start, string_view attribute, const Rml::Dictionary& parameters, string_view source_name);
		bool run_element_script_event(string_view id, string_view attribute, const Rml::Dictionary& parameters, string_view source_name);

		sol::state lua;
		view<window> display;
		Rml::ElementDocument* document = nullptr;
		string title_text;
		std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
		std::chrono::steady_clock::time_point last_update_callback = start_time;
		struct PendingScript {
			string source_name;
			string script;
		};
		std::vector<PendingScript> pending_scripts;
		std::unique_ptr<ScriptEventListener> script_events;
		bool script_events_bound = false;
	};
	unique_ptr<Scene> make_scene(Rml::Context& context, view<window> display, string_view initial);
} // namespace lf
