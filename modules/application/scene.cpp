#include "leaf/application/scene.hpp"

#include "scene_systems.hpp"

#include <leaf/application/rml_backend.hpp>
#include <leaf/core/exception.hpp>
#include <leaf/core/format.hpp>
#include <leaf/core/logging.hpp>
#include <leaf/graphics/framebuffer.hpp>
#include <leaf/graphics/queue.hpp>
#include <leaf/graphics/texture_view.hpp>
#include <leaf/platform/platform.hpp>
#include <leaf/script/settings.hpp>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Input.h>

namespace lf {
	static Rml::Input::KeyIdentifier rml_key(rt::input_key key) {
		if (key >= rt::KEY_A && key <= rt::KEY_Z) return static_cast<Rml::Input::KeyIdentifier>(Rml::Input::KI_A + key - rt::KEY_A);
		if (key >= rt::KEY_0 && key <= rt::KEY_9) return static_cast<Rml::Input::KeyIdentifier>(Rml::Input::KI_0 + key - rt::KEY_0);
		if (key >= rt::KEY_F1 && key <= rt::KEY_F24) return static_cast<Rml::Input::KeyIdentifier>(Rml::Input::KI_F1 + key - rt::KEY_F1);
		switch (key) {
		case rt::KEY_ESCAPE: return Rml::Input::KI_ESCAPE;
		case rt::KEY_ENTER: return Rml::Input::KI_RETURN;
		case rt::KEY_TAB: return Rml::Input::KI_TAB;
		case rt::KEY_SPACE: return Rml::Input::KI_SPACE;
		case rt::KEY_BACKSPACE: return Rml::Input::KI_BACK;
		case rt::KEY_DELETE: return Rml::Input::KI_DELETE;
		case rt::KEY_LEFT_ARROW: return Rml::Input::KI_LEFT;
		case rt::KEY_RIGHT_ARROW: return Rml::Input::KI_RIGHT;
		case rt::KEY_UP_ARROW: return Rml::Input::KI_UP;
		case rt::KEY_DOWN_ARROW: return Rml::Input::KI_DOWN;
		case rt::KEY_HOME: return Rml::Input::KI_HOME;
		case rt::KEY_END: return Rml::Input::KI_END;
		case rt::KEY_PAGE_UP: return Rml::Input::KI_PRIOR;
		case rt::KEY_PAGE_DOWN: return Rml::Input::KI_NEXT;
		default: return Rml::Input::KI_UNKNOWN;
		}
	}

	static int rml_modifiers(rt::input_modifiers modifiers) {
		int result = 0;
		if (modifiers.has(rt::INPUT_MODIFIER_CTRL)) result |= Rml::Input::KM_CTRL;
		if (modifiers.has(rt::INPUT_MODIFIER_SHIFT)) result |= Rml::Input::KM_SHIFT;
		if (modifiers.has(rt::INPUT_MODIFIER_ALT)) result |= Rml::Input::KM_ALT;
		if (modifiers.has(rt::INPUT_MODIFIER_SUPER)) result |= Rml::Input::KM_META;
		return result;
	}

	static int rml_button(rt::input_button button) {
		switch (button) {
		case rt::BUTTON_LEFT: return 0;
		case rt::BUTTON_RIGHT: return 1;
		case rt::BUTTON_MIDDLE: return 2;
		default: return static_cast<int>(button - rt::BUTTON_1);
		}
	}

	static string key_name(rt::input_key key) {
		if (key >= rt::KEY_A && key <= rt::KEY_Z) return string{ 1, static_cast<char>('a' + key - rt::KEY_A) };
		if (key >= rt::KEY_0 && key <= rt::KEY_9) return string{ 1, static_cast<char>('0' + key - rt::KEY_0) };
		if (key >= rt::KEY_F1 && key <= rt::KEY_F24) return format("f{}", static_cast<i32>(key - rt::KEY_F1 + 1));
		switch (key) {
		case rt::KEY_ESCAPE: return "escape";
		case rt::KEY_ENTER: return "enter";
		case rt::KEY_TAB: return "tab";
		case rt::KEY_SPACE: return "space";
		case rt::KEY_BACKSPACE: return "backspace";
		case rt::KEY_DELETE: return "delete";
		case rt::KEY_LEFT_ARROW: return "left";
		case rt::KEY_RIGHT_ARROW: return "right";
		case rt::KEY_UP_ARROW: return "up";
		case rt::KEY_DOWN_ARROW: return "down";
		case rt::KEY_HOME: return "home";
		case rt::KEY_END: return "end";
		case rt::KEY_PAGE_UP: return "page-up";
		case rt::KEY_PAGE_DOWN: return "page-down";
		case rt::KEY_HASH: return "#";
		default: return {};
		}
	}

	Scene::Scene(Window& display)
		: display(display) {
		const dim2<u32> size = this->display.size();
		context = Rml::CreateContext("scene", { static_cast<i32>(size.width), static_cast<i32>(size.height) });
		if (!context) throw runtime_exception("failed to create RML scene context");
	}

	Scene::~Scene() {
		unload_document();
		if (context) Rml::RemoveContext(context->GetName());
	}

	void Scene::show() {
		if (rml_document) rml_document->Show();
		display.show();
	}

	void Scene::set_rml(string_view source) {
		unload_document();
		rml_document = context->LoadDocumentFromMemory(Rml::String{ source });
		if (!rml_document) throw runtime_exception("failed to load RML document");
		auto* scene_document = rmlui_dynamic_cast<SceneDocument*>(rml_document);
		if (!scene_document) throw runtime_exception("scene requires SceneDocument");
		report<vector<SceneDocumentScript>> scripts = scene_document->take_scripts();
		if (!scripts) throw runtime_exception(scripts.error().message);
		for (SceneDocumentScript& script : *scripts) {
			document_scripts.push_back({ std::move(script.source_name), std::move(script.source) });
		}
		for (Rml::EventId event : { Rml::EventId::Click, Rml::EventId::Change, Rml::EventId::Mousedown, Rml::EventId::Mousemove, Rml::EventId::Mouseup }) {
			rml_document->AddEventListener(event, this);
		}
		rml_document->Show();
	}

	Rml::ElementDocument& Scene::document() {
		if (!rml_document) throw runtime_exception("scene has no RML document");
		return *rml_document;
	}

	sol::state& Scene::script_state() {
		return lua;
	}

	error Scene::execute_document_scripts() {
		for (const ScriptSource& script : document_scripts) {
			if (!execute_script(script.text, script.name)) return error{ generic_errc::parse_error, format("RML script '{}' failed", script.name) };
		}
		document_scripts.clear();
		return {};
	}

	void Scene::set_render_rate(frequency rate) {
		frame_rate.limit(rate);
	}

	frequency Scene::render_rate() const {
		return frequency::from_hertz(frame_rate.rate());
	}

	Window& Scene::window() {
		return display;
	}

	const Window& Scene::window() const {
		return display;
	}

	void Scene::ProcessEvent(Rml::Event& event) {
		const Rml::String& name = event.GetType();
		for (Rml::Element* element = event.GetTargetElement(); element; element = element->GetParentNode()) {
			const Rml::String source = element->GetAttribute<Rml::String>(name, "");
			if (!source.empty()) {
				execute_script(source, name);
				return;
			}
		}
	}

	bool Scene::execute_script(string_view source, string_view source_name) {
		sol::protected_function_result result = lua.safe_script(string{ source }, sol::script_pass_on_error);
		if (result.valid()) return true;
		sol::error error = result;
		log::Error("[scene] {}: {}", source_name, error.what());
		return false;
	}

	void Scene::input() {
		if (!rml_document) return;
		for (const rt::input_event& event : display.input_events()) {
			switch (event.type) {
			case INPUT_EVENT_CONTROL:
				if (event.control.type == INPUT_CONTROL_BUTTON) {
					const int button = rml_button(static_cast<rt::input_button>(event.control.value));
					if (event.state == input_state::Pressed) context->ProcessMouseButtonDown(button, rml_modifiers(event.modifiers));
					if (event.state == input_state::Released) context->ProcessMouseButtonUp(button, rml_modifiers(event.modifiers));
				} else if (event.control.type == INPUT_CONTROL_KEY) {
					const rt::input_key code = static_cast<rt::input_key>(event.control.value);
					const Rml::Input::KeyIdentifier key = rml_key(code);
					if (key != Rml::Input::KI_UNKNOWN) {
						if (event.state == input_state::Pressed) context->ProcessKeyDown(key, rml_modifiers(event.modifiers));
						if (event.state == input_state::Released) context->ProcessKeyUp(key, rml_modifiers(event.modifiers));
					}
					const string event_name = event.state == input_state::Pressed ? "keydown" : event.state == input_state::Released ? "keyup" : "";
					if (!event_name.empty()) {
						Rml::ElementList bindings;
						rml_document->GetElementsByTagName(bindings, "keybind");
						for (Rml::Element* binding : bindings) {
							string binding_key = binding->GetAttribute<Rml::String>("key", "");
							if (binding_key.empty()) {
								const string action = binding->GetAttribute<Rml::String>("action", "");
								if (!action.empty()) {
									report<string> configured = LoadInputSetting("core", action);
									if (configured) binding_key = std::move(*configured);
								}
							}
							const Rml::String source = binding->GetAttribute<Rml::String>(event_name, "");
							if (!source.empty() && binding_key == key_name(code)) execute_script(source, event_name);
						}
					}
				}
				break;
			case INPUT_EVENT_POINTER_MOVE:
				context->ProcessMouseMove(static_cast<i32>(event.position.x), static_cast<i32>(event.position.y), rml_modifiers(event.modifiers));
				break;
			case INPUT_EVENT_SCROLL:
				context->ProcessMouseWheel({ -event.delta.x, -event.delta.y }, rml_modifiers(event.modifiers));
				break;
			case INPUT_EVENT_TEXT:
				context->ProcessTextInput(static_cast<Rml::Character>(event.character));
				break;
			case INPUT_EVENT_POINTER_ENTER:
			case INPUT_EVENT_FOCUS:
				if (event.state == input_state::Up) context->ProcessMouseLeave();
				break;
			case INPUT_EVENT_DROP:
				break;
			}
		}
		display.update_input();
	}

	void Scene::render() {
		if (!display.drawable()) return;
		const dim2<u32> size = display.size();
		if (context->GetDimensions() != Rml::Vector2i{ static_cast<i32>(size.width), static_cast<i32>(size.height) }) {
			context->SetDimensions({ static_cast<i32>(size.width), static_cast<i32>(size.height) });
		}
		const rt::view<rt::command_buffer> commands = display.begin_frame();
		if (!commands) return;
		context->Update();
		rml_renderer().begin(commands, size);
		context->Render();
		rml_renderer().end();
		display.end_frame();
	}

	bool Scene::update() {
		frame_rate.wait();
		input();
		render();
		frame_rate.mark();
		return !display.should_close();
	}

	void Scene::unload_document() {
		if (rml_document) context->UnloadDocument(rml_document);
		rml_document = nullptr;
		document_scripts.clear();
		lua = CreateState();
	}
} // namespace lf
