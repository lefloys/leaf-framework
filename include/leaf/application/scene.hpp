#pragma once

#include <leaf/core/memory.hpp>
#include <leaf/core/rate.hpp>
#include <leaf/core/span.hpp>
#include <leaf/core/string.hpp>
#include <leaf/graphics/window.hpp>
#include <leaf/script/state.hpp>

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/EventListener.h>
#include <sol/sol.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace Rml {
	class Context;
	class Element;
	class ElementDocument;
	class ElementFormControl;
	class Event;
} // namespace Rml

namespace lf {
	class Scene {
	  public:
		Scene();
		Scene(rt::handle<rt::window> display);
		Scene(Rml::Context& context, rt::handle<rt::window> display);
		Scene(Scene&&) noexcept = delete;
		Scene& operator=(Scene&&) noexcept = delete;
		~Scene();

		struct LoadRequest {
			string path;
			string args;
		};
		struct PendingScript {
			string source_name;
			string script;
		};

		void launch(string_view initial, string_view args = {}, script_installer installer = {});


		void launch(rt::handle<rt::window> display, string_view initial, string_view args = {}, script_installer installer = {});


		rt::unique<rt::window> release_window();


		rt::view<rt::window> window_view() const;




		void set_render_rate(double hz);

		double render_rate_hz() const;


		void stop();


		bool update();


		void input(const rt::input_event& event);


		void process_input();

		void resize_context();

		void render(rt::view<rt::command_buffer> cmd);


		bool render_frame();


		bool has_text_input_focus() const;

		bool accepts_text_input(u32 character) const;


		string filter_text_input(string_view text) const;


		void load(string_view initial, string_view args = {});

		void set_rml(string_view id, string_view rml);

		void set_attribute(string_view id, string_view name, string_view value);

		/*!
		** @brief Sets a numeric attribute on an RML element.
		*/
		void set_attribute(string_view id, string_view name, f32 value);


		void queue_script(string_view script, string_view source_name);


		std::optional<LoadRequest> take_load_request();

		void request_load(string_view path, string_view args = {});

		void refresh_title();

		string_view title() const;

	  private:
		class ScriptEventListener final : public Rml::EventListener {
		  public:
			explicit ScriptEventListener(Scene& scene);
			void ProcessEvent(Rml::Event& event) override;

		  private:
			Scene& scene;
		};

		void unload_document();
		void bind_script_events();
		void clear_script_events();
		void run_inline_scripts(string_view source);
		void run_script(string_view script, string_view source_name);
		bool execute_script(string_view script, string_view source_name);
		bool run_pending_scripts();
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

		void start_threads();
		void render_loop(std::stop_token stop);

		sol::state lua;
		Rml::Context* context = nullptr;
		string owned_context_name;
		rt::unique<rt::window> display;
		string scene_args;
		Rml::ElementDocument* document = nullptr;
		string title_text;
		std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

		std::optional<LoadRequest> pending_load_request;
		script_installer installer;
		std::vector<PendingScript> pending_scripts;
		std::unique_ptr<ScriptEventListener> script_events;

		CursorPrototype::ID active_pressed_cursor;
		Rml::Vector2f last_mouse_position = { 0.0f, 0.0f };
		bool has_mouse_position = false;
		bool script_events_bound = false;

		mutable std::recursive_mutex scene_mutex;

		std::atomic<double> configured_render_hz{ 0.0 };
		RateMeter render_rate;
		RateMeter update_rate;
		double applied_update_hz = -1.0;
		std::jthread render_thread;
		std::atomic_bool threads_started{ false };
	};

} // namespace lf
