#include "application.hpp"

#include "application_stats.hpp"
#include "rml_window.hpp"

#include <leaf/core/filesystem.hpp>
#include <leaf/core/format.hpp>
#include <leaf/logging/logging.hpp>
#include <leaf/core/profiler.hpp>
#include <leaf/graphics/timepoint.hpp>
#include <leaf/leaf.hpp>
#include <leaf/platform/platform.hpp>
#include <leaf/script/virtual_filesystem.hpp>

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

namespace lf {
	namespace {
		struct RenderProfile {
			u32 frames = 0;
			f64 begin_frame = 0.0;
			f64 rml_lock_wait = 0.0;
			f64 input = 0.0;
			f64 rml_update = 0.0;
			f64 renderer_begin = 0.0;
			f64 rml_render = 0.0;
			f64 renderer_end = 0.0;
			f64 end_frame = 0.0;
			f64 frame = 0.0;
			f64 max_frame = 0.0;

			void record(f64& bucket, std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end) {
				bucket += std::chrono::duration<f64, std::milli>(end - start).count();
			}

			void clear() {
				*this = {};
			}
		};

		void log_render_profile(string_view message) {
			log::Debug("{}", message);
			static std::ofstream file(fs::folder::appdata / "render_profile.log", std::ios::app);
			if (file) {
				file << message << '\n';
				file.flush();
			}
		}

		string lua_quote(string_view value) {
			string quoted = "\"";
			for (char c : value) {
				switch (c) {
				case '\\': quoted += "\\\\"; break;
				case '"': quoted += "\\\""; break;
				case '\n': quoted += "\\n"; break;
				case '\r': quoted += "\\r"; break;
				case '\t': quoted += "\\t"; break;
				default: quoted += c; break;
				}
			}
			quoted += '"';
			return quoted;
		}

		string inject_scene_args(string scene_source, string_view args_yaml) {
			string script = format("<script type=\"text/lua\">scene_args_yaml={}</script>", lua_quote(args_yaml));
			size_t head = scene_source.find("<head>");
			if (head != string::npos) {
				scene_source.insert(head + 6, script);
				return scene_source;
			}
			scene_source.insert(0, script);
			return scene_source;
		}

	} // namespace

	static Rml::Input::KeyIdentifier rml_key(input_key key) {
		if (key >= KEY_A && key <= KEY_Z) {
			return static_cast<Rml::Input::KeyIdentifier>(Rml::Input::KI_A + key - KEY_A);
		}
		if (key >= KEY_0 && key <= KEY_9) {
			return static_cast<Rml::Input::KeyIdentifier>(Rml::Input::KI_0 + key - KEY_0);
		}
		if (key >= KEY_NUMPAD_0 && key <= KEY_NUMPAD_9) {
			return static_cast<Rml::Input::KeyIdentifier>(Rml::Input::KI_NUMPAD0 + key - KEY_NUMPAD_0);
		}
		if (key >= KEY_F1 && key <= KEY_F24) {
			return static_cast<Rml::Input::KeyIdentifier>(Rml::Input::KI_F1 + key - KEY_F1);
		}

		switch (key) {
		case KEY_BROWSER_BACK: return Rml::Input::KI_BROWSER_BACK;
		case KEY_BROWSER_FORWARD: return Rml::Input::KI_BROWSER_FORWARD;
		case KEY_BROWSER_REFRESH: return Rml::Input::KI_BROWSER_REFRESH;
		case KEY_VOLUME_MUTE: return Rml::Input::KI_VOLUME_MUTE;
		case KEY_VOLUME_DOWN: return Rml::Input::KI_VOLUME_DOWN;
		case KEY_VOLUME_UP: return Rml::Input::KI_VOLUME_UP;
		case KEY_MEDIA_NEXT_TRACK: return Rml::Input::KI_MEDIA_NEXT_TRACK;
		case KEY_MEDIA_PREV_TRACK: return Rml::Input::KI_MEDIA_PREV_TRACK;
		case KEY_MEDIA_STOP: return Rml::Input::KI_MEDIA_STOP;
		case KEY_MEDIA_PLAY_PAUSE: return Rml::Input::KI_MEDIA_PLAY_PAUSE;
		case KEY_LAUNCH_MAIL: return Rml::Input::KI_LAUNCH_MAIL;
		case KEY_CONTEXT_MENU: return Rml::Input::KI_APPS;
		case KEY_EXECUTE: return Rml::Input::KI_EXECUTE;
		case KEY_HELP: return Rml::Input::KI_HELP;
		case KEY_PAUSE: return Rml::Input::KI_PAUSE;
		case KEY_PRINT: return Rml::Input::KI_SNAPSHOT;
		case KEY_SELECT: return Rml::Input::KI_SELECT;
		case KEY_ESCAPE: return Rml::Input::KI_ESCAPE;
		case KEY_HOME: return Rml::Input::KI_HOME;
		case KEY_PAGE_DOWN: return Rml::Input::KI_NEXT;
		case KEY_PAGE_UP: return Rml::Input::KI_PRIOR;
		case KEY_DOWN_ARROW: return Rml::Input::KI_DOWN;
		case KEY_LEFT_ARROW: return Rml::Input::KI_LEFT;
		case KEY_RIGHT_ARROW: return Rml::Input::KI_RIGHT;
		case KEY_UP_ARROW: return Rml::Input::KI_UP;
		case KEY_ALT_LEFT: return Rml::Input::KI_LMENU;
		case KEY_ALT_RIGHT: return Rml::Input::KI_RMENU;
		case KEY_CTRL_LEFT: return Rml::Input::KI_LCONTROL;
		case KEY_CTRL_RIGHT: return Rml::Input::KI_RCONTROL;
		case KEY_SHIFT_LEFT: return Rml::Input::KI_LSHIFT;
		case KEY_SHIFT_RIGHT: return Rml::Input::KI_RSHIFT;
		case KEY_SUPER_LEFT: return Rml::Input::KI_LMETA;
		case KEY_SUPER_RIGHT: return Rml::Input::KI_RMETA;
		case KEY_NUM_LOCK: return Rml::Input::KI_NUMLOCK;
		case KEY_SCROLL_LOCK: return Rml::Input::KI_SCROLL;
		case KEY_CAPS_LOCK: return Rml::Input::KI_CAPITAL;
		case KEY_BACKSPACE: return Rml::Input::KI_BACK;
		case KEY_CLEAR: return Rml::Input::KI_CLEAR;
		case KEY_END: return Rml::Input::KI_END;
		case KEY_DELETE: return Rml::Input::KI_DELETE;
		case KEY_INSERT: return Rml::Input::KI_INSERT;
		case KEY_NUMPAD_ADD: return Rml::Input::KI_ADD;
		case KEY_NUMPAD_DECIMAL: return Rml::Input::KI_DECIMAL;
		case KEY_NUMPAD_DIVIDE: return Rml::Input::KI_DIVIDE;
		case KEY_NUMPAD_ENTER: return Rml::Input::KI_NUMPADENTER;
		case KEY_NUMPAD_MULTIPLY: return Rml::Input::KI_MULTIPLY;
		case KEY_NUMPAD_SUBTRACT: return Rml::Input::KI_SUBTRACT;
		case KEY_BACKQUOTE: return Rml::Input::KI_OEM_3;
		case KEY_BACKSLASH: return Rml::Input::KI_OEM_5;
		case KEY_BRACKET_LEFT: return Rml::Input::KI_OEM_4;
		case KEY_BRACKET_RIGHT: return Rml::Input::KI_OEM_6;
		case KEY_COMMA: return Rml::Input::KI_OEM_COMMA;
		case KEY_ENTER: return Rml::Input::KI_RETURN;
		case KEY_EQUAL: return Rml::Input::KI_OEM_PLUS;
		case KEY_MINUS: return Rml::Input::KI_OEM_MINUS;
		case KEY_PERIOD: return Rml::Input::KI_OEM_PERIOD;
		case KEY_QUOTE: return Rml::Input::KI_OEM_7;
		case KEY_SEMICOLON: return Rml::Input::KI_OEM_1;
		case KEY_SLASH: return Rml::Input::KI_OEM_2;
		case KEY_SPACE: return Rml::Input::KI_SPACE;
		case KEY_TAB: return Rml::Input::KI_TAB;
		default: return Rml::Input::KI_UNKNOWN;
		}
	}

	static int rml_modifiers(input_modifiers modifiers) {
		int rml = 0;
		if (modifiers.has(INPUT_MODIFIER_CTRL)) {
			rml |= Rml::Input::KM_CTRL;
		}
		if (modifiers.has(INPUT_MODIFIER_SHIFT)) {
			rml |= Rml::Input::KM_SHIFT;
		}
		if (modifiers.has(INPUT_MODIFIER_ALT)) {
			rml |= Rml::Input::KM_ALT;
		}
		if (modifiers.has(INPUT_MODIFIER_SUPER)) {
			rml |= Rml::Input::KM_META;
		}
		return rml;
	}

	static bool key_generates_text(input_key key) {
		if ((key >= KEY_A && key <= KEY_Z) ||
			(key >= KEY_0 && key <= KEY_9) ||
			(key >= KEY_NUMPAD_0 && key <= KEY_NUMPAD_9)) {
			return true;
		}

		switch (key) {
		case KEY_BACKQUOTE:
		case KEY_BACKSLASH:
		case KEY_BRACKET_LEFT:
		case KEY_BRACKET_RIGHT:
		case KEY_COMMA:
		case KEY_EQUAL:
		case KEY_MINUS:
		case KEY_NUMPAD_ADD:
		case KEY_NUMPAD_DECIMAL:
		case KEY_NUMPAD_DIVIDE:
		case KEY_NUMPAD_MULTIPLY:
		case KEY_NUMPAD_SUBTRACT:
		case KEY_PERIOD:
		case KEY_QUOTE:
		case KEY_SEMICOLON:
		case KEY_SLASH:
		case KEY_SPACE:
			return true;
		default:
			return false;
		}
	}

	static bool text_key_should_skip_control_event(input_key key, input_modifiers modifiers) {
		if (modifiers.has(INPUT_MODIFIER_CTRL) || modifiers.has(INPUT_MODIFIER_ALT) || modifiers.has(INPUT_MODIFIER_SUPER)) {
			return false;
		}
		return key_generates_text(key);
	}

	static int rml_button(input_button button) {
		switch (button) {
		case BUTTON_LEFT: return 0;
		case BUTTON_RIGHT: return 1;
		case BUTTON_MIDDLE: return 2;
		default: return static_cast<int>(button - BUTTON_1);
		}
	}

	static input_key input_key_from_control(input_control control) {
		return control.type == INPUT_CONTROL_KEY ? static_cast<input_key>(control.value) : KEY_NULL;
	}

	static input_button input_button_from_control(input_control control) {
		return control.type == INPUT_CONTROL_BUTTON ? static_cast<input_button>(control.value) : BUTTON_NULL;
	}

	static bool is_paste_shortcut(input_key key, input_modifiers modifiers) {
		return key == KEY_V && (modifiers.has(INPUT_MODIFIER_CTRL) || modifiers.has(INPUT_MODIFIER_SUPER));
	}

	Application::Application() : Application(ApplicationCreateInfo{}) {}

	Application::Application(const ApplicationCreateInfo& create_info)
		: display(Window::Create()),
		  update_interval(update_interval_for(create_info.updates_per_second)) {
		RegisterRmlWindowElement();
		set_updates_per_second(create_info.updates_per_second);
		Window::SetTitle(display, create_info.title);
		window_title = string(create_info.title);
		Window::SetWidth(display, create_info.width);
		Window::SetHeight(display, create_info.height);

		context_name = "leaf-application";
		context = Rml::CreateContext(
			context_name,
			Rml::Vector2i(static_cast<int>(create_info.width), static_cast<int>(create_info.height)));
		if (!context) {
			throw runtime_exception("failed to create RmlUi application context");
		}
	}

	Application::Application(handle<lf::window> display)
		: Application(display, ApplicationCreateInfo{}) {}

	Application::Application(handle<lf::window> display, const ApplicationCreateInfo& create_info)
		: display(display),
		  update_interval(update_interval_for(create_info.updates_per_second)) {
		RegisterRmlWindowElement();
		set_updates_per_second(create_info.updates_per_second);
		Window::SetShouldClose(this->display, false);
		Window::SetTitle(this->display, create_info.title);
		dim2<u32> size = Window::Size(this->display);
		context_name = "leaf-application";
		window_title = string(create_info.title);
		context = Rml::CreateContext(
			context_name,
			Rml::Vector2i(static_cast<int>(size.width), static_cast<int>(size.height)));
		if (!context) {
			throw runtime_exception("failed to create RmlUi application context");
		}
	}

	Application::~Application() {
		stop_threads();
		wait_for_render_idle();
		if (context) {
			std::lock_guard lock(rml_mutex);
			loaded_scene.reset();
			Rml::RemoveContext(context->GetName());
			context = nullptr;
		}
	}

	error Application::launch(string_view scene_source) {
		if (running) {
			return error::no_error;
		}

		load_scene(scene_source);
		{
			std::lock_guard lock(window_mutex);
			Window::ApplyFullscreenRequest(display);
			Window::Show(display);
		}
		running = true;
		render_thread = std::jthread(&Application::render_thread_main, std::ref(*this));
		update_thread = std::jthread(&Application::update_thread_main, std::ref(*this));
		return error::no_error;
	}

	void Application::close() {
		stop_threads();
		wait_for_render_idle();
	}

	void Application::stop_threads() {
		render_thread.request_stop();
		update_thread.request_stop();
		running = false;

		render_thread = {};
		update_thread = {};
	}

	void Application::wait_for_render_idle() {
		handle<queue> graphics_queue = Queue::Query(QueueCapability::Graphics);
		Queue::Flush(graphics_queue);
	}

	handle<lf::window> Application::release_window() {
		stop_threads();
		wait_for_render_idle();
		if (context) {
			std::lock_guard lock(rml_mutex);
			loaded_scene.reset();
			Rml::RemoveContext(context_name);
			context = nullptr;
		}
		return display.release();
	}

	void Application::set_rml(string_view id, string_view rml) {
		std::lock_guard lock(rml_mutex);
		if (loaded_scene) {
			loaded_scene->set_rml(id, rml);
		}
	}

	void Application::set_attribute(string_view id, string_view name, string_view value) {
		std::lock_guard lock(rml_mutex);
		if (loaded_scene) {
			loaded_scene->set_attribute(id, name, value);
		}
	}

	void Application::set_attribute(string_view id, string_view name, f32 value) {
		std::lock_guard lock(rml_mutex);
		if (loaded_scene) {
			loaded_scene->set_attribute(id, name, value);
		}
	}

	void Application::run_script(string_view script) {
		std::lock_guard lock(rml_mutex);
		if (loaded_scene) {
			loaded_scene->queue_script(string(script), "lua-console");
		}
	}

	void Application::set_max_fps(f32 value) {
		stats.max_fps.store(value, std::memory_order_relaxed);
	}

	f32 Application::max_fps() const {
		return stats.max_fps.load(std::memory_order_relaxed);
	}

	void Application::set_updates_per_second(u32 value) {
		stats.updates_per_second.store(value, std::memory_order_relaxed);
		update_interval = update_interval_for(value);
	}

	u32 Application::updates_per_second() const {
		return stats.updates_per_second.load(std::memory_order_relaxed);
	}

	void Application::add_rml_element_installer(ElementInstaller installer) {
		rml_element_installers.push_back(std::move(installer));
	}

	void Application::add_scene_script_installer(Scene::ScriptInstaller installer) {
		scene_script_installers.push_back(std::move(installer));
	}

	void Application::add_scene_fixed_updater(Scene::FixedUpdater updater) {
		scene_fixed_updaters.push_back(std::move(updater));
	}

	f32 Application::current_fps() const {
		return stats.current_fps.load(std::memory_order_relaxed);
	}

	f32 Application::current_ups() const {
		return stats.current_ups.load(std::memory_order_relaxed);
	}

	void Application::set_render_profile_enabled(bool enabled) {
		stats.render_profile_enabled.store(enabled, std::memory_order_relaxed);
	}

	bool Application::render_profile_enabled() const {
		return stats.render_profile_enabled.load(std::memory_order_relaxed);
	}

	bool Application::update() {
		bool should_run = running;
		{
			std::lock_guard lock(window_mutex);
			if (!render_frame_active) {
				Window::ApplyFullscreenRequest(display);
			}
			should_run = should_run && !Window::ShouldClose(display);
		}
		return should_run;
	}

	bool Application::is_running() const {
		return running && !Window::ShouldClose(display);
	}

	void Application::load_scene(string_view scene_source) {
		load_scene(scene_source, {});
	}

	void Application::load_scene(string_view scene_source, string_view args_yaml) {
		log::Trace("{}", format("[scene] loading scene ({} bytes, args {} bytes, installers {}, fixed updaters {})",
								scene_source.size(),
								args_yaml.size(),
								scene_script_installers.size(),
								scene_fixed_updaters.size()));
		wait_for_render_idle();
		std::vector<Scene::PersistentElement> persistent_elements;
		if (loaded_scene) {
			log::Trace("{}", "[scene] releasing current scene");
			persistent_elements = loaded_scene->release_persistent_elements();
			loaded_scene.reset();
		}
		for (const ElementInstaller& installer : rml_element_installers) {
			installer();
		}
		std::vector<Scene::ScriptInstaller> script_installers;
		script_installers.reserve(scene_script_installers.size() + 1);
		script_installers.push_back([this](sol::state& lua, Rml::ElementDocument&) {
			sol::object existing = lua.globals()["game"];
			sol::table game = existing.is<sol::table>() ? existing.as<sol::table>() : lua.create_table();
			lua.globals()["game"] = game;
			lua["_G"]["game"] = game;
			game["speed"] = [this](sol::object) -> f32 {
				return stats.speed_multiplier.load(std::memory_order_relaxed);
			};
			game["set_speed"] = [this](sol::object, f32 value) {
				stats.speed_multiplier.store(std::max(0.0f, value), std::memory_order_relaxed);
			};
		});
		script_installers.insert(script_installers.end(), scene_script_installers.begin(), scene_script_installers.end());

		loaded_scene = make_scene(
			*context,
			display,
			stats,
			scene_source,
			args_yaml,
			std::move(persistent_elements),
			std::move(script_installers),
			scene_fixed_updaters);
		window_title = string(loaded_scene->title());
		Window::SetTitle(display, window_title);
		log::Trace("{}", format("[scene] loaded '{}'", window_title));
	}

	Scene* Application::scene() {
		return loaded_scene.get();
	}

	const Scene* Application::scene() const {
		return loaded_scene.get();
	}

	Rml::Context* Application::rml_context() {
		return context;
	}

	const Rml::Context* Application::rml_context() const {
		return context;
	}

	view<lf::window> Application::window() {
		return display;
	}

	view<const lf::window> Application::window() const {
		return display;
	}

	void Application::render_thread_main(std::stop_token stop, Application& app) {
		using namespace std::chrono_literals;
		handle<queue> graphics_queue = Queue::Query(QueueCapability::Graphics);
		if (std::getenv("LEAF_RENDER_PROFILE")) {
			app.set_render_profile_enabled(true);
		}
		if (std::getenv("LEAF_PROFILE")) {
			SetProfilerEnabled(true);
		}
		auto next_frame = std::chrono::steady_clock::now();
		auto fps_sample_start = next_frame;
		u32 fps_sample_frames = 0;
		dim2<u32> context_size = {};
		RenderProfile profile;
		auto profile_start = next_frame;
		auto not_drawable_log = next_frame;
		while (!stop.stop_requested()) {
			auto frame_start = std::chrono::steady_clock::now();
			bool rendered_frame = false;
			Rml::Context* context = app.rml_context();
			if (context) {
				LF_PROFILE_SCOPE("Application::RenderFrame");
				dim2<u32> window_size{};
				view<command_buffer> cmd;
				bool drawable = true;
				bool fullscreen_pending = false;
				{
					std::lock_guard window_lock(app.window_mutex);
					if (Window::FullscreenRequestPending(app.window())) {
						app.render_frame_active = false;
						fullscreen_pending = true;
					} else if (!Window::Drawable(app.window())) {
						app.render_frame_active = false;
						drawable = false;
					} else {
						app.render_frame_active = true;
						window_size = Window::Size(app.window());
					}
				}
				if (fullscreen_pending) {
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}
				if (drawable) {
					auto begin_start = std::chrono::steady_clock::now();
					{
						LF_PROFILE_SCOPE("Application::BeginFrame");
						cmd = Window::BeginFrame(app.window(), graphics_queue);
					}
					auto begin_end = std::chrono::steady_clock::now();
					profile.record(profile.begin_frame, begin_start, begin_end);
				}
				if (!drawable) {
					if (app.render_profile_enabled()) {
						auto now = std::chrono::steady_clock::now();
						if (now - not_drawable_log >= 1s) {
							log_render_profile("[profile] skipped render: window is not drawable");
							not_drawable_log = now;
						}
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(16));
					continue;
				}
				if (!cmd) {
					std::lock_guard window_lock(app.window_mutex);
					app.render_frame_active = false;
					std::this_thread::yield();
					continue;
				}
				std::vector<input_event> input;
				{
					LF_PROFILE_SCOPE("Application::CollectInput");
					input = Window::InputEvents(app.window());
				}
				auto lock_wait_start = std::chrono::steady_clock::now();
				std::unique_lock lock(app.rml_mutex);
				auto lock_wait_end = std::chrono::steady_clock::now();
				profile.record(profile.rml_lock_wait, lock_wait_start, lock_wait_end);
				Scene* scene = app.scene();
				if (!scene) {
					lock.unlock();
					Window::EndFrame(app.window());
					{
						std::lock_guard window_lock(app.window_mutex);
						app.render_frame_active = false;
					}
					continue;
				}
				auto input_start = std::chrono::steady_clock::now();
				for (const input_event& event : input) {
					switch (event.type) {
					case INPUT_EVENT_CONTROL: {
						if (event.control.type == INPUT_CONTROL_BUTTON) {
							input_button button = input_button_from_control(event.control);
							if (event.state == input_state::Pressed) {
								context->ProcessMouseButtonDown(rml_button(button), rml_modifiers(event.modifiers));
							} else if (event.state == input_state::Released) {
								context->ProcessMouseButtonUp(rml_button(button), rml_modifiers(event.modifiers));
							}
						} else if (event.control.type == INPUT_CONTROL_KEY) {
							input_key input = input_key_from_control(event.control);
							Rml::Input::KeyIdentifier key = rml_key(input);
							if (key == Rml::Input::KI_UNKNOWN) {
								break;
							}
							if (event.state == input_state::Pressed) {
								if (is_paste_shortcut(input, event.modifiers)) {
									string clipboard_text = platform_clipboard_text();
									string filtered_text = scene->filter_text_input(clipboard_text);
									if (filtered_text != clipboard_text) {
										for (char character : filtered_text) {
											context->ProcessTextInput(static_cast<Rml::Character>(character));
										}
										break;
									}
								}
								const bool text_only_key = scene->has_text_input_focus() && text_key_should_skip_control_event(input, event.modifiers);
								if (!text_only_key) {
									context->ProcessKeyDown(key, rml_modifiers(event.modifiers));
								}
								if ((input == KEY_ENTER || input == KEY_NUMPAD_ENTER) && scene->accepts_text_input('\n')) {
									context->ProcessTextInput('\n');
								}
							} else if (event.state == input_state::Released) {
								const bool text_only_key = scene->has_text_input_focus() && text_key_should_skip_control_event(input, event.modifiers);
								if (!text_only_key) {
									context->ProcessKeyUp(key, rml_modifiers(event.modifiers));
								}
							}
						}
						break;
					}
					case INPUT_EVENT_POINTER_MOVE:
						context->ProcessMouseMove(
							static_cast<int>(event.position.x),
							static_cast<int>(event.position.y),
							rml_modifiers(event.modifiers));
						break;
					case INPUT_EVENT_POINTER_ENTER:
						if (event.state == input_state::Up) {
							context->ProcessMouseLeave();
						}
						break;
					case INPUT_EVENT_SCROLL:
						context->ProcessMouseWheel(
							Rml::Vector2f{ -event.delta.x, -event.delta.y },
							rml_modifiers(event.modifiers));
						break;
					case INPUT_EVENT_TEXT:
						if (scene->accepts_text_input(event.character)) {
							context->ProcessTextInput(static_cast<Rml::Character>(event.character));
						}
						break;
					case INPUT_EVENT_FOCUS:
					case INPUT_EVENT_DROP:
						break;
					}
					scene->input(event);
				}
				Window::UpdateInput(app.window());
				auto input_end = std::chrono::steady_clock::now();
				profile.record(profile.input, input_start, input_end);
				if (window_size.width != context_size.width || window_size.height != context_size.height) {
					context->SetDimensions({ static_cast<int>(window_size.width), static_cast<int>(window_size.height) });
					context_size = window_size;
				}
				auto update_start = std::chrono::steady_clock::now();
				{
					LF_PROFILE_SCOPE("Rml::Context::Update");
					context->Update();
				}
				auto update_end = std::chrono::steady_clock::now();
				profile.record(profile.rml_update, update_start, update_end);
				auto renderer_begin_start = std::chrono::steady_clock::now();
				{
					LF_PROFILE_SCOPE("RmlRenderer::Begin");
					rml_renderer().begin(cmd, window_size);
				}
				auto renderer_begin_end = std::chrono::steady_clock::now();
				profile.record(profile.renderer_begin, renderer_begin_start, renderer_begin_end);
				auto render_start = std::chrono::steady_clock::now();
				{
					LF_PROFILE_SCOPE("Rml::Context::Render");
					context->Render();
				}
				auto render_end = std::chrono::steady_clock::now();
				profile.record(profile.rml_render, render_start, render_end);
				auto renderer_end_start = std::chrono::steady_clock::now();
				{
					LF_PROFILE_SCOPE("RmlRenderer::End");
					rml_renderer().end();
				}
				auto renderer_end_end = std::chrono::steady_clock::now();
				profile.record(profile.renderer_end, renderer_end_start, renderer_end_end);
				lock.unlock();
				auto end_start = std::chrono::steady_clock::now();
				{
					LF_PROFILE_SCOPE("Application::EndFrame");
					Window::EndFrame(app.window());
				}
				auto end_end = std::chrono::steady_clock::now();
				profile.record(profile.end_frame, end_start, end_end);
				{
					std::lock_guard window_lock(app.window_mutex);
					app.render_frame_active = false;
				}
				rendered_frame = true;
			}

			auto now = std::chrono::steady_clock::now();
			if (rendered_frame) {
				f64 frame_ms = std::chrono::duration<f64, std::milli>(now - frame_start).count();
				profile.frame += frame_ms;
				profile.max_frame = std::max(profile.max_frame, frame_ms);
				++profile.frames;
				++fps_sample_frames;
				f64 elapsed = std::chrono::duration<f64>(now - fps_sample_start).count();
				if (elapsed >= 0.25) {
					app.stats.current_fps.store(static_cast<f32>(static_cast<f64>(fps_sample_frames) / elapsed), std::memory_order_relaxed);
					fps_sample_start = now;
					fps_sample_frames = 0;
				}
				f64 profile_elapsed = std::chrono::duration<f64>(now - profile_start).count();
				if (app.render_profile_enabled() && profile_elapsed >= 1.0 && profile.frames > 0) {
					const f64 inv = 1.0 / static_cast<f64>(profile.frames);
					log_render_profile(format(
						"[profile] fps={:.1f} frame={:.3f} max={:.3f} begin={:.3f} rml-lock={:.3f} input={:.3f} update={:.3f} rb={:.3f} render={:.3f} re={:.3f} present={:.3f}",
						static_cast<f64>(profile.frames) / profile_elapsed,
						profile.frame * inv,
						profile.max_frame,
						profile.begin_frame * inv,
						profile.rml_lock_wait * inv,
						profile.input * inv,
						profile.rml_update * inv,
						profile.renderer_begin * inv,
						profile.rml_render * inv,
						profile.renderer_end * inv,
						profile.end_frame * inv));
					profile.clear();
					profile_start = now;
				}
			}

			const f64 max_fps = static_cast<f64>(app.max_fps());
			if (max_fps > 0.0 && max_fps < 1000.0) {
				const auto frame_duration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
					std::chrono::duration<f64>(1.0 / max_fps));
				next_frame += frame_duration;
				now = std::chrono::steady_clock::now();
				if (next_frame > now) {
					std::this_thread::sleep_until(next_frame);
				} else {
					next_frame = now;
					std::this_thread::yield();
				}
			} else {
				next_frame = std::chrono::steady_clock::now();
			}
		}
	}

	void Application::update_thread_main(std::stop_token stop, Application& app) {
		using namespace std::chrono_literals;
		auto next_update = std::chrono::steady_clock::now();
		auto ups_sample_start = std::chrono::steady_clock::now();
		u32 ups_sample_updates = 0;
		f64 fixed_update_credit = 0.0;
		while (!stop.stop_requested()) {
			u32 fixed_updates_run = 0;
			Rml::Context* context = app.rml_context();
			if (context) {
				std::lock_guard lock(app.rml_mutex);
				Scene* scene = app.scene();
				if (!scene) {
					fixed_updates_run = 0;
				} else {
					if (std::optional<Scene::LoadRequest> request = scene->take_load_request()) {
						report<string> scene_source = ReadVirtualTextFile(request->path);
						if (!scene_source) {
							log::Error("{}", format("[scene] {}: {}", request->path, scene_source.error().message));
						} else {
							app.load_scene(inject_scene_args(std::move(*scene_source), request->args_yaml), request->args_yaml);
							scene = app.scene();
						}
					}
					if (!scene) {
						fixed_updates_run = 0;
						continue;
					}
					scene->refresh_title();
					string title(scene->title());
					{
						std::lock_guard window_lock(app.window_mutex);
						if (!title.empty() && title != app.window_title) {
							Window::SetTitle(app.display, title);
							app.window_title = std::move(title);
						}
					}
					fs::path lua_console_path = fs::folder::appdata / "lua_console.in";
					if (std::filesystem::exists(lua_console_path)) {
						std::ifstream file(lua_console_path, std::ios::binary);
						if (file) {
							string script(
								(std::istreambuf_iterator<char>(file)),
								std::istreambuf_iterator<char>());
							file.close();
							std::filesystem::remove(lua_console_path);
							if (!script.empty()) {
								log::Debug("{}", format("[lua-console] executing {}", lua_console_path.string()));
								scene->queue_script(std::move(script), "lua-console");
							}
						}
					}
					LF_PROFILE_SCOPE("Scene::Update");
					scene->update();
					const f32 speed_multiplier = std::max(0.0f, app.stats.speed_multiplier.load(std::memory_order_relaxed));
					const bool fixed_updates_enabled = app.stats.updates_per_second.load(std::memory_order_relaxed) > 0 && speed_multiplier > 0.0f;
					if (fixed_updates_enabled) {
						LF_PROFILE_SCOPE("Scene::FixedUpdate");
						fixed_update_credit += static_cast<f64>(speed_multiplier);
						const u32 max_updates_per_loop = 256;
						while (fixed_update_credit >= 1.0 && fixed_updates_run < max_updates_per_loop) {
							scene->fixed_update();
							++fixed_updates_run;
							fixed_update_credit -= 1.0;
						}
						if (fixed_updates_run == max_updates_per_loop) {
							fixed_update_credit = 0.0;
						}
					} else {
						fixed_update_credit = 0.0;
						app.stats.current_ups.store(0.0f, std::memory_order_relaxed);
					}
				}
			}
			if (fixed_updates_run > 0) {
				ups_sample_updates += fixed_updates_run;
				auto now = std::chrono::steady_clock::now();
				f64 elapsed = std::chrono::duration<f64>(now - ups_sample_start).count();
				if (elapsed >= 0.25) {
					app.stats.current_ups.store(static_cast<f32>(static_cast<f64>(ups_sample_updates) / elapsed), std::memory_order_relaxed);
					ups_sample_start = now;
					ups_sample_updates = 0;
				}
			}
			next_update += app.update_interval;
			auto now = std::chrono::steady_clock::now();
			if (next_update > now) {
				std::this_thread::sleep_until(next_update);
			} else {
				next_update = now;
				std::this_thread::yield();
			}
		}
	}

	std::chrono::steady_clock::duration Application::update_interval_for(u32 updates_per_second) {
		if (updates_per_second == 0) {
			updates_per_second = DefaultApplicationUpdatesPerSecond;
		}
		return std::chrono::duration_cast<std::chrono::steady_clock::duration>(
			std::chrono::duration<f64>(1.0 / static_cast<f64>(updates_per_second)));
	}
} // namespace lf
