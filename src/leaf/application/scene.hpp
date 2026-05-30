#pragma once

#include <leaf/application/application_stats.hpp>
#include <leaf/core/memory.hpp>
#include <leaf/core/string.hpp>
#include <leaf/graphics/window.hpp>

#include <RmlUi/Core/Element.h>
#include <sol/sol.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace Rml {
	class Context;
	class Element;
	class ElementDocument;
	class ElementFormControl;
	class Event;
} // namespace Rml

namespace lf {
	struct input_event;

	class Scene {
	  public:
		struct LoadRequest {
			string path;
			string args_yaml;
		};

		struct PersistentElement {
			string id;
			Rml::ElementPtr element;
		};

		Scene(
			Rml::Context& context,
			view<window> display,
			ApplicationStats& stats,
			string_view initial,
			string_view args_yaml = {},
			std::vector<PersistentElement> persistent_elements = {}
		);
		~Scene();

		void update();
		void input(const input_event& event);
		bool has_text_input_focus() const;
		bool accepts_text_input(u32 character) const;
		string filter_text_input(string_view text) const;
		void fixed_update();
		void set_rml(string_view id, string_view rml);
		void set_attribute(string_view id, string_view name, string_view value);
		void set_attribute(string_view id, string_view name, f32 value);
		void queue_script(string script, string source_name);
		std::optional<LoadRequest> take_load_request();
		std::vector<PersistentElement> release_persistent_elements();
		void refresh_title();
		string_view title() const;

		using ScriptInstaller = std::function<void(sol::state&, Rml::ElementDocument&)>;
		static void RegisterScriptInstaller(ScriptInstaller installer);
		using FixedUpdater = std::function<void(Rml::ElementDocument&)>;
		static void RegisterFixedUpdater(FixedUpdater updater);

	  private:
		class ScriptEventListener;

		void bind_script_events();
		void clear_script_events();
		void run_inline_scripts(string_view source);
		void run_script(string_view script, string_view source_name);
		bool execute_script(string_view script, string_view source_name);
		bool run_pending_scripts();
		void restore_persistent_elements(std::vector<PersistentElement> persistent_elements);
		void install_ui_automation_helpers();
		void update_ui_automation(f64 elapsed);
		void update_cursor(Rml::Element* start, bool pressed, bool released = false);
		void update_cursor_at_last_mouse();
		Rml::Element* find_element(string_view id) const;
		Rml::Element* find_selector(string_view selector, i32 index) const;
		Rml::Element* require_element(string_view id, string_view api_name) const;
		Rml::ElementFormControl* require_form_control(string_view id, string_view api_name) const;
		bool run_element_script_event(Rml::Element* start, string_view attribute, const Rml::Dictionary& parameters, string_view source_name);
		bool run_element_script_event(string_view id, string_view attribute, const Rml::Dictionary& parameters, string_view source_name);

		sol::state lua;
		view<window> display;
		ApplicationStats& stats;
		string scene_args_yaml;
		std::optional<LoadRequest> pending_load_request;
		Rml::ElementDocument* document = nullptr;
		string title_text;
		std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
		struct PendingScript {
			string source_name;
			string script;
		};
		std::vector<PendingScript> pending_scripts;
		std::unique_ptr<ScriptEventListener> script_events;
		string active_pressed_cursor;
		Rml::Vector2f last_mouse_position = { 0.0f, 0.0f };
		bool has_mouse_position = false;
		bool script_events_bound = false;
	};
	unique_ptr<Scene> make_scene(
		Rml::Context& context,
		view<window> display,
		ApplicationStats& stats,
		string_view initial,
		string_view args_yaml = {},
		std::vector<Scene::PersistentElement> persistent_elements = {}
	);
} // namespace lf
