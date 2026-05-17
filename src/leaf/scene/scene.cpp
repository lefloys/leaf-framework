#include "scene.hpp"

#include "rml_backend.hpp"

#include <leaf/core/exception.hpp>
#include <leaf/core/format.hpp>
#include <leaf/core/messages.hpp>
#include <leaf/graphics/command_buffer.hpp>

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/Factory.h>

#include <algorithm>
#include <utility>

namespace lf {
	class LoadingBarElement : public Rml::Element {
	  public:
		explicit LoadingBarElement(const Rml::String& tag) : Rml::Element(tag) {}

		void set_progress(f32 next_progress) {
			progress = std::clamp(next_progress, 0.0f, 1.0f);
			ensure_fill();
			if (world) {
				world->SetProperty("width", format("{:.3f}%", progress * 100.0f));
			}
		}

	  private:
		void ensure_fill() {
			if (world) {
				return;
			}
			SetInnerRML("<div class=\"loading-bar-fill\"></div>");
			world = QuerySelector(".loading-bar-fill");
			if (world) {
				world->SetProperty("height", "100%");
				world->SetProperty("width", "0%");
			}
		}

		Rml::Element* world = nullptr;
		f32 progress = 0.0f;
	};

	class LoadingBarInstancer : public Rml::ElementInstancer {
	  public:
		Rml::ElementPtr InstanceElement(Rml::Element* parent, const Rml::String& tag,
										const Rml::XMLAttributes& attributes) override {
			(void)parent;
			(void)attributes;
			auto element = Rml::ElementPtr(new LoadingBarElement(tag));
			static_cast<LoadingBarElement*>(element.get())->set_progress(0.0f);
			return element;
		}

		void ReleaseElement(Rml::Element* element) override {
			delete element;
		}
	};

	class LuaEventListener : public Rml::EventListener {
	  public:
		explicit LuaEventListener(std::function<void()> callback) : callback(std::move(callback)) {}

		void ProcessEvent(Rml::Event& event) override {
			(void)event;
			callback();
		}

	  private:
		std::function<void()> callback;
	};

	std::unique_ptr<LoadingBarInstancer> loading_bar_instancer;

	string lua_value_to_string(sol::object value) {
		if (value.is<sol::nil_t>()) {
			return "nil";
		}
		if (value.is<bool>()) {
			return value.as<bool>() ? "true" : "false";
		}
		if (value.is<double>()) {
			return std::to_string(value.as<double>());
		}
		if (value.is<string>()) {
			return value.as<string>();
		}
		return format("<{}>", static_cast<int>(value.get_type()));
	}

	void ensure_loading_bar_element_registered() {
		if (loading_bar_instancer) {
			return;
		}
		loading_bar_instancer = std::make_unique<LoadingBarInstancer>();
		Rml::Factory::RegisterElementInstancer("loading-bar", loading_bar_instancer.get());
	}

	Scene::Scene() = default;

	Scene::~Scene() {
		shutdown();
	}

	error Scene::load(const fs::path& rml_path) {
		shutdown();

		initialize_rml();

		context = Rml::CreateContext("minimal-scene", Rml::Vector2i(1280, 720));
		if (!context) {
			return error(generic_errc::unknown, "failed to create RmlUi context");
		}
		document = context->LoadDocument(rml_path.string());
		if (!document) {
			return error(generic_errc::unknown, "failed to load RmlUi scene");
		}
		error script_error = load_script(rml_path);
		if (script_error) {
			return script_error;
		}
		bind_script_events();
		document->Show();
		return error::no_error;
	}

	error Scene::load_memory(string_view rml, string_view source_name, string_view lua_source) {
		shutdown();
		initialize_rml();

		context = Rml::CreateContext("minimal-scene", Rml::Vector2i(1280, 720));
		if (!context) {
			return error(generic_errc::unknown, "failed to create RmlUi context");
		}
		document = context->LoadDocumentFromMemory(Rml::String(rml), Rml::String(source_name));
		if (!document) {
			return error(generic_errc::unknown, "failed to load RmlUi entry scene");
		}
		return finish_loaded_document(lua_source, "entry.lua");
	}

	void Scene::set_lua_binder(std::function<void(sol::state&)> binder) {
		lua_binder = std::move(binder);
	}

	void Scene::set_rml_binder(std::function<void()> binder) {
		rml_binder = std::move(binder);
	}

	void Scene::process_input(view<window> window) {
		if (!context || !window) {
			return;
		}
		pos2<f32> mouse = Window::MousePosition(window);
		context->ProcessMouseMove(static_cast<int>(mouse.x), static_cast<int>(mouse.y), 0);

		const MouseButton buttons[] = { MouseButton::Left, MouseButton::Right, MouseButton::Middle };
		for (int i = 0; i < 3; ++i) {
			if (Window::MousePressed(window, buttons[i])) {
				context->ProcessMouseButtonDown(i, 0);
			}
			if (Window::MouseReleased(window, buttons[i])) {
				context->ProcessMouseButtonUp(i, 0);
			}
		}

		f32 scroll = Window::Scroll(window);
		if (scroll != 0.0f) {
			context->ProcessMouseWheel(scroll, 0);
		}
	}

	void Scene::update(f64 delta_seconds) {
		invoke_script_function_if_present("update", delta_seconds);
		if (context) {
			context->Update();
		}
	}

	void Scene::render(view<command_buffer> command_buffer, view<framebuffer> framebuffer, dim2<u32> framebuffer_size) {
		if (!context || !command_buffer || !framebuffer) {
			return;
		}
		context->SetDimensions(Rml::Vector2i(static_cast<int>(framebuffer_size.width), static_cast<int>(framebuffer_size.height)));
		rml_render_interface().begin(command_buffer, framebuffer_size);
		context->Render();
		rml_render_interface().end();
	}

	void Scene::initialize_rml() {
		ensure_loading_bar_element_registered();
		if (rml_binder) {
			rml_binder();
		}
	}

	error Scene::finish_loaded_document(string_view lua_source, string_view lua_source_name) {
		error script_error = lua_source.empty() ? error::no_error : load_script_source(lua_source, lua_source_name);
		if (script_error) {
			return script_error;
		}
		bind_script_events();
		document->Show();
		invoke_script_function_if_present("init");
		return error::no_error;
	}

	error Scene::load_script(const fs::path& rml_path) {
		initialize_lua();

		fs::path script_path = rml_path;
		script_path.replace_extension(".lua");
		if (!fs::exists(script_path)) {
			return error::no_error;
		}
		sol::load_result loaded = lua.load_file(script_path.string());
		if (!loaded.valid()) {
			sol::error err = loaded;
			return error(generic_errc::unknown, string("failed to load scene Lua script: ") + err.what());
		}
		sol::protected_function_result result = loaded();
		if (!result.valid()) {
			sol::error err = result;
			return error(generic_errc::unknown, string("failed to run scene Lua script: ") + err.what());
		}
		return error::no_error;
	}

	error Scene::load_script_source(string_view source, string_view source_name) {
		initialize_lua();

		sol::load_result loaded = lua.load_buffer(source.data(), source.size(), string(source_name));
		if (!loaded.valid()) {
			sol::error err = loaded;
			return error(generic_errc::unknown, string("failed to load scene Lua script: ") + err.what());
		}
		sol::protected_function_result result = loaded();
		if (!result.valid()) {
			sol::error err = result;
			return error(generic_errc::unknown, string("failed to run scene Lua script: ") + err.what());
		}
		return error::no_error;
	}

	void Scene::initialize_lua() {
		lua = sol::state();
		lua.open_libraries(sol::lib::base);
		lua.set_function("print", [](sol::variadic_args args) {
			string message = "[scene lua]";
			for (sol::object value : args) {
				message += " ";
				message += lua_value_to_string(value);
			}
			log_info(message);
		});
		bind_lua_api();
		if (lua_binder) {
			lua_binder(lua);
		}
	}

	void Scene::bind_lua_api() {
		sol::table scene_table = lua.create_table();
		scene_table.set_function("set_text", [this](string element_id, string text) {
			set_text(element_id, text);
		});
		scene_table.set_function("set_position", [this](string element_id, f32 x, f32 y) {
			set_position(element_id, x, y);
		});
		scene_table.set_function("set_progress", [this](string element_id, f32 progress) {
			set_progress(element_id, progress);
		});
		lua["scene"] = scene_table;
	}

	void Scene::bind_script_events() {
		if (!document) {
			return;
		}
		Rml::ElementList elements;
		document->QuerySelectorAll(elements, "[data-click], [data-mousedown], [data-mouseup]");
		for (Rml::Element* element : elements) {
			add_lua_event_listener(element, "click", "data-click");
			add_lua_event_listener(element, "mousedown", "data-mousedown");
			add_lua_event_listener(element, "mouseup", "data-mouseup");
		}
	}

	void Scene::add_lua_event_listener(Rml::Element* element, string_view event_name, string_view attribute_name) {
		string function_name = element->GetAttribute<Rml::String>(Rml::String(attribute_name), "");
		if (function_name.empty()) {
			return;
		}
		auto listener = std::make_unique<LuaEventListener>(
			[this, function_name]() { invoke_script_function(function_name); });
		element->AddEventListener(Rml::String(event_name), listener.get());
		event_listeners.push_back(std::move(listener));
	}

	void Scene::set_text(string_view element_id, string_view text) {
		if (!document || element_id.empty()) {
			return;
		}
		Rml::Element* element = document->GetElementById(Rml::String(element_id));
		if (!element) {
			log_warning(format("[scene lua] missing element '{}'", element_id));
			return;
		}
		element->SetInnerRML(Rml::String(text));
	}

	void Scene::set_position(string_view element_id, f32 x, f32 y) {
		if (!document || element_id.empty()) {
			return;
		}
		Rml::Element* element = document->GetElementById(Rml::String(element_id));
		if (!element) {
			log_warning(format("[scene lua] missing element '{}'", element_id));
			return;
		}
		element->SetProperty("left", format("{:.0f}px", x));
		element->SetProperty("top", format("{:.0f}px", y));
	}

	void Scene::set_progress(string_view element_id, f32 progress) {
		set_loading_bar_progress(element_id, progress);
	}

	void Scene::set_loading_bar_progress(string_view element_id, f32 progress) {
		if (!document || element_id.empty()) {
			return;
		}
		Rml::Element* element = document->GetElementById(Rml::String(element_id));
		auto* loading_bar = dynamic_cast<LoadingBarElement*>(element);
		if (!loading_bar) {
			log_warning(format("[scene lua] missing loading-bar '{}'", element_id));
			return;
		}
		loading_bar->set_progress(progress);
	}

	void Scene::invoke_script_function(string_view function_name) {
		if (function_name.empty()) {
			return;
		}
		sol::object object = lua[string(function_name)];
		if (!object.is<sol::protected_function>()) {
			log_warning(format("[scene lua] missing function '{}'", function_name));
			return;
		}
		sol::protected_function function = object.as<sol::protected_function>();
		sol::protected_function_result result = function();
		if (!result.valid()) {
			sol::error err = result;
			log_error(format("[scene lua] error in '{}': {}", function_name, err.what()));
		}
	}

	void Scene::invoke_script_function_if_present(string_view function_name, f64 argument) {
		if (function_name.empty()) {
			return;
		}
		sol::object object = lua[string(function_name)];
		if (!object.is<sol::protected_function>()) {
			return;
		}
		sol::protected_function function = object.as<sol::protected_function>();
		sol::protected_function_result result = function(argument);
		if (!result.valid()) {
			sol::error err = result;
			log_error(format("[scene lua] error in '{}': {}", function_name, err.what()));
		}
	}

	void Scene::shutdown() {
		if (context) {
			if (document) {
				document->Close();
				document = nullptr;
				context->Update();
			} else {
				context->UnloadAllDocuments();
				context->Update();
			}
			Rml::RemoveContext(context->GetName());
			context = nullptr;
		}
		document = nullptr;
		event_listeners.clear();
		lua = sol::state();
	}
} // namespace lf
