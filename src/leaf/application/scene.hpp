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

	/*!
	** @ingroup application
	** @brief Runtime representation of an RML/Lua scene.
	**
	** A scene owns the Lua state for the loaded document, dispatches script
	** events, applies queued UI edits, handles input, and reports requests to
	** transition to another scene.
	*/
	class Scene {
	  public:
		/*!
		** @brief Deferred request to replace the current scene.
		*/
		struct LoadRequest {
			/*!
			** @brief Scene path or identifier to load.
			*/
			string path;

			/*!
			** @brief YAML argument data passed to the next scene.
			*/
			string args_yaml;
		};

		/*!
		** @brief RML element that survives a scene reload.
		*/
		struct PersistentElement {
			/*!
			** @brief Element id used to restore the persistent element.
			*/
			string id;

			/*!
			** @brief Owned RML element tree.
			*/
			Rml::ElementPtr element;
		};

		/*!
		** @brief Callback used to install project-specific Lua bindings.
		*/
		using ScriptInstaller = std::function<void(sol::state&, Rml::ElementDocument&)>;

		/*!
		** @brief Callback invoked during fixed scene updates.
		*/
		using FixedUpdater = std::function<void(Rml::ElementDocument&)>;

		/*!
		** @brief Loads a scene into an existing RML context.
		** @param context RML context that owns the scene document.
		** @param display Window used for input and cursor state.
		** @param stats Shared application statistics.
		** @param initial Initial scene source path or identifier.
		** @param args_yaml YAML argument data exposed to the scene script.
		** @param persistent_elements Elements to restore into the new document.
		*/
		Scene(
			Rml::Context& context,
			view<window> display,
			ApplicationStats& stats,
			string_view initial,
			string_view args_yaml = {},
			std::vector<PersistentElement> persistent_elements = {},
			std::vector<ScriptInstaller> script_installers = {},
			std::vector<FixedUpdater> fixed_updaters = {}
		);

		/*!
		** @brief Releases the loaded document, Lua state, and script listeners.
		*/
		~Scene();

		/*!
		** @brief Updates dynamic UI and queued script work.
		*/
		void update();

		/*!
		** @brief Dispatches a platform input event to the scene.
		** @param event Input event to process.
		*/
		void input(const input_event& event);

		/*!
		** @brief Checks whether an RML control currently owns text focus.
		*/
		bool has_text_input_focus() const;

		/*!
		** @brief Checks whether a Unicode character can be accepted as text input.
		*/
		bool accepts_text_input(u32 character) const;

		/*!
		** @brief Filters text input through the currently focused control.
		*/
		string filter_text_input(string_view text) const;

		/*!
		** @brief Runs one fixed-rate scene update.
		*/
		void fixed_update();

		/*!
		** @brief Replaces the RML markup for an element.
		*/
		void set_rml(string_view id, string_view rml);

		/*!
		** @brief Sets a string attribute on an RML element.
		*/
		void set_attribute(string_view id, string_view name, string_view value);

		/*!
		** @brief Sets a numeric attribute on an RML element.
		*/
		void set_attribute(string_view id, string_view name, f32 value);

		/*!
		** @brief Queues Lua source for execution by the scene.
		** @param script Lua source text.
		** @param source_name Name used in diagnostics for the script.
		*/
		void queue_script(string script, string source_name);

		/*!
		** @brief Takes a pending scene-load request if one was made.
		*/
		std::optional<LoadRequest> take_load_request();

		/*!
		** @brief Releases elements marked as persistent before a scene reload.
		*/
		std::vector<PersistentElement> release_persistent_elements();

		/*!
		** @brief Refreshes the cached scene title from the document state.
		*/
		void refresh_title();

		/*!
		** @brief Gets the current scene title.
		*/
		string_view title() const;

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
		std::vector<ScriptInstaller> script_installers;
		std::vector<FixedUpdater> fixed_updaters;
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

	/*!
	** @ingroup application
	** @brief Creates a scene with the standard Leaf scene allocation path.
	** @param context RML context that owns the scene document.
	** @param display Window used for input and cursor state.
	** @param stats Shared application statistics.
	** @param initial Initial scene source path or identifier.
	** @param args_yaml YAML argument data exposed to the scene script.
	** @param persistent_elements Elements to restore into the new document.
	** @return Newly allocated scene.
	*/
	unique_ptr<Scene> make_scene(
		Rml::Context& context,
		view<window> display,
		ApplicationStats& stats,
		string_view initial,
		string_view args_yaml = {},
		std::vector<Scene::PersistentElement> persistent_elements = {},
		std::vector<Scene::ScriptInstaller> script_installers = {},
		std::vector<Scene::FixedUpdater> fixed_updaters = {}
	);
} // namespace lf
