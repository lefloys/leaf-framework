#include "scene.hpp"
#include "cursor.hpp"
#include "elements/window.hpp"

#include <leaf/application/application_stats.hpp>
#include <leaf/application/sound.hpp>
#include <leaf/core/exception.hpp>
#include <leaf/core/format.hpp>
#include <leaf/core/memory.hpp>
#include <leaf/core/messages.hpp>
#include <leaf/core/profiler.hpp>
#include <leaf/graphics/graphics.hpp>
#include <leaf/graphics/window.hpp>
#include <leaf/logging/logging.hpp>
#include <leaf/script/localization.hpp>
#include <leaf/script/mod_loader.hpp>
#include <leaf/script/prototype_inspector_script.hpp>
#include <leaf/script/settings.hpp>
#include <leaf/script/virtual_filesystem.hpp>

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <functional>
#include <utility>
#include <unordered_map>

namespace lf {
	namespace {
		string rml_escape(string_view value) {
			string escaped;
			escaped.reserve(value.size());
			for (char c : value) {
				switch (c) {
				case '&': escaped += "&amp;"; break;
				case '<': escaped += "&lt;"; break;
				case '>': escaped += "&gt;"; break;
				case '"': escaped += "&quot;"; break;
				case '\'': escaped += "&#39;"; break;
				default: escaped += c; break;
				}
			}
			return escaped;
		}

		string lua_value_to_string(const sol::object& value) {
			switch (value.get_type()) {
			case sol::type::lua_nil: return "nil";
			case sol::type::boolean: return value.as<bool>() ? "true" : "false";
			case sol::type::number: return format("{}", value.as<double>());
			case sol::type::string: return value.as<string>();
			default: return format("{}", sol::type_name(value.lua_state(), value.get_type()));
			}
		}

		string lua_escape(string_view value) {
			string escaped;
			escaped.reserve(value.size());
			for (char c : value) {
				switch (c) {
				case '\\': escaped += "\\\\"; break;
				case '"': escaped += "\\\""; break;
				case '\n': escaped += "\\n"; break;
				case '\r': escaped += "\\r"; break;
				case '\t': escaped += "\\t"; break;
				default: escaped += c; break;
				}
			}
			return escaped;
		}

		bool element_persists_for_scene(const Rml::Element& element) {
			return element.GetAttribute<Rml::String>("persist", "") == "scene";
		}

		void copy_persistent_placeholder_attributes(Rml::Element& element, const Rml::Element& placeholder) {
			element.SetAttributes(placeholder.GetAttributes());
		}

		string filter_text_for_element(const Rml::Element& element, string_view text) {
			if (element.GetAttribute<Rml::String>("input-filter", "") != "digits") {
				return string(text);
			}

			string filtered;
			filtered.reserve(text.size());
			for (char c : text) {
				if (c >= '0' && c <= '9') {
					filtered += c;
				}
			}
			return filtered;
		}

		Rml::Dictionary lua_event_parameters(sol::optional<sol::table> table) {
			Rml::Dictionary parameters;
			if (!table) {
				return parameters;
			}

			for (const auto& entry : *table) {
				if (!entry.first.is<string>()) {
					continue;
				}

				Rml::String key = entry.first.as<string>();
				const sol::object& value = entry.second;
				if (value.is<bool>()) {
					parameters[key] = Rml::Variant(value.as<bool>());
				} else if (value.is<i32>()) {
					parameters[key] = Rml::Variant(value.as<i32>());
				} else if (value.is<f32>()) {
					parameters[key] = Rml::Variant(value.as<f32>());
				} else if (value.is<f64>()) {
					parameters[key] = Rml::Variant(value.as<f64>());
				} else if (value.is<string>()) {
					parameters[key] = Rml::Variant(Rml::String(value.as<string>()));
				}
			}
			return parameters;
		}

		Rml::Dictionary mouse_parameters(Rml::Element& element, f32 x, f32 y, i32 button = 0) {
			Rml::Dictionary parameters;
			if (x < 0.0f) {
				x = element.GetAbsoluteLeft() + element.GetOffsetWidth() * 0.5f;
			}
			if (y < 0.0f) {
				y = element.GetAbsoluteTop() + element.GetOffsetHeight() * 0.5f;
			}
			parameters["mouse_x"] = Rml::Variant(x);
			parameters["mouse_y"] = Rml::Variant(y);
			parameters["button"] = Rml::Variant(button);
			return parameters;
		}

		string lua_event_table(string_view type, std::initializer_list<std::pair<string_view, string>> fields) {
			string script = format("event={{type=\"{}\"", lua_escape(type));
			for (const auto& [name, value] : fields) {
				script += format(",{}={}", name, value);
			}
			script += "};";
			return script;
		}

		string lua_event_number(f32 value) {
			return format("{}", value);
		}

		string lua_event_number(i32 value) {
			return format("{}", value);
		}

		string lua_event_string(string_view value) {
			return format("\"{}\"", lua_escape(value));
		}

		bool element_has_script_event(Rml::Element* start, string_view attribute) {
			for (Rml::Element* element = start; element; element = element->GetParentNode()) {
				if (!element->GetAttribute<Rml::String>(Rml::String(attribute), "").empty()) {
					return true;
				}
			}
			return false;
		}

		string normalized_key_name(string_view value) {
			string normalized;
			normalized.reserve(value.size());
			for (char c : value) {
				if (c == '_') {
					normalized += '-';
				} else if (c >= 'A' && c <= 'Z') {
					normalized += static_cast<char>(c - 'A' + 'a');
				} else {
					normalized += c;
				}
			}
			if (normalized.starts_with("key-")) {
				normalized.erase(0, 4);
			}
			return normalized;
		}

		string normalized_action_name(string_view value) {
			string normalized;
			normalized.reserve(value.size());
			for (char c : value) {
				if (c == '-') {
					normalized += '_';
				} else if (c >= 'a' && c <= 'z') {
					normalized += static_cast<char>(c - 'a' + 'A');
				} else {
					normalized += c;
				}
			}
			return normalized;
		}

		string input_binding_for_action(string_view action) {
			const string action_name(action);
			auto core_binding = LoadInputSetting("core", action_name);
			if (core_binding && !core_binding->empty()) {
				return *core_binding;
			}
			if (!core_binding) {
				log::Warning("{}", format("[settings] {}", core_binding.error().message));
			}

			for (const ModInfo& mod : LoadedMods()) {
				if (mod.name == "core") {
					continue;
				}
				auto binding = LoadInputSetting(mod.name, action_name);
				if (!binding) {
					log::Warning("{}", format("[settings] {}", binding.error().message));
					continue;
				}
				if (!binding->empty()) {
					return *binding;
				}
			}

			return {};
		}

		string key_name(input_key key) {
			if (key >= KEY_A && key <= KEY_Z) {
				return string(1, static_cast<char>('a' + key - KEY_A));
			}
			if (key >= KEY_0 && key <= KEY_9) {
				return string(1, static_cast<char>('0' + key - KEY_0));
			}
			if (key >= KEY_F1 && key <= KEY_F24) {
				return format("f{}", static_cast<int>(key - KEY_F1 + 1));
			}

			switch (key) {
			case KEY_ESCAPE: return "escape";
			case KEY_TAB: return "tab";
			case KEY_ENTER: return "enter";
			case KEY_SPACE: return "space";
			case KEY_BACKSPACE: return "backspace";
			case KEY_DELETE: return "delete";
			case KEY_INSERT: return "insert";
			case KEY_HOME: return "home";
			case KEY_END: return "end";
			case KEY_PAGE_UP: return "page-up";
			case KEY_PAGE_DOWN: return "page-down";
			case KEY_LEFT_ARROW: return "left";
			case KEY_RIGHT_ARROW: return "right";
			case KEY_UP_ARROW: return "up";
			case KEY_DOWN_ARROW: return "down";
			case KEY_ALT_LEFT: return "alt-left";
			case KEY_ALT_RIGHT: return "alt-right";
			case KEY_CTRL_LEFT: return "ctrl-left";
			case KEY_CTRL_RIGHT: return "ctrl-right";
			case KEY_SHIFT_LEFT: return "shift-left";
			case KEY_SHIFT_RIGHT: return "shift-right";
			case KEY_SUPER_LEFT: return "super-left";
			case KEY_SUPER_RIGHT: return "super-right";
			case KEY_BACKQUOTE: return "backquote";
			case KEY_BACKSLASH: return "backslash";
			case KEY_BRACKET_LEFT: return "bracket-left";
			case KEY_BRACKET_RIGHT: return "bracket-right";
			case KEY_COMMA: return "comma";
			case KEY_EQUAL: return "equal";
			case KEY_HASH: return "hash";
			case KEY_MINUS: return "minus";
			case KEY_PERIOD: return "period";
			case KEY_QUOTE: return "quote";
			case KEY_SEMICOLON: return "semicolon";
			case KEY_SLASH: return "slash";
			default: return {};
			}
		}

		bool keybind_matches_text(const Rml::Element& keybind, u32 character) {
			string key = keybind.GetAttribute<Rml::String>("key", "");
			if (key.empty()) {
				key = keybind.GetAttribute<Rml::String>("character", "");
			}
			if (key.empty()) {
				return false;
			}
			if (key.size() == 1) {
				return static_cast<u32>(static_cast<unsigned char>(key[0])) == character;
			}

			string normalized = normalized_key_name(key);
			return (character == '#' && normalized == "hash") ||
				   (character == ' ' && normalized == "space");
		}

		bool keybind_matches_key(const Rml::Element& keybind, input_key key) {
			string requested = keybind.GetAttribute<Rml::String>("key", "");
			if (requested.empty()) {
				return false;
			}

			string actual = key_name(key);
			return !actual.empty() && normalized_key_name(requested) == actual;
		}

		bool keybind_matches_action_name(const Rml::Element& keybind, string_view actual_key, string& out_action) {
			string requested = keybind.GetAttribute<Rml::String>("action", "");
			if (requested.empty()) {
				return false;
			}

			const string action = normalized_action_name(requested);
			const string bound_key = input_binding_for_action(action);
			if (bound_key.empty()) {
				return false;
			}

			if (actual_key.empty() || normalized_key_name(bound_key) != normalized_key_name(actual_key)) {
				return false;
			}
			out_action = action;
			return true;
		}

		bool keybind_matches_action(const Rml::Element& keybind, input_key key, string& out_action) {
			return keybind_matches_action_name(keybind, key_name(key), out_action);
		}

		string text_key_name(u32 character) {
			if (character == '#') {
				return "hash";
			}
			if (character == ' ') {
				return "space";
			}
			if (character > 0 && character <= 0x7f) {
				return string(1, static_cast<char>(character));
			}
			return {};
		}

		void collect_keybinds(Rml::Element& root, Rml::ElementList& out) {
			out.clear();
			for (int index = 0; index < root.GetNumChildren(); ++index) {
				Rml::Element* child = root.GetChild(index);
				if (child && child->GetTagName() == "keybind") {
					out.push_back(child);
				}
			}
		}

		void collect_focused_keybinds(Rml::Element& focused, Rml::ElementList& out) {
			out.clear();
			if (!focused.GetAttribute<Rml::String>("keydown", "").empty() ||
				!focused.GetAttribute<Rml::String>("keyup", "").empty()) {
				out.push_back(&focused);
			}
			for (int index = 0; index < focused.GetNumChildren(); ++index) {
				Rml::Element* child = focused.GetChild(index);
				if (child && child->GetTagName() == "keybind") {
					out.push_back(child);
				}
			}
		}

		bool input_scope_candidate(const Rml::Element& element) {
			return element.GetTagName() == "window" ||
				element.GetTagName() == "body" ||
				element.HasAttribute("tabindex");
		}

		Rml::Element* containing_input_scope(Rml::ElementDocument& document, Rml::Element& focused) {
			if (input_scope_candidate(focused)) {
				return &focused;
			}
			for (Rml::Element* element = &focused; element; element = element->GetParentNode()) {
				if (element == &focused) {
					continue;
				}
				if (input_scope_candidate(*element)) {
					return element;
				}
			}
			return &document;
		}

		Rml::ElementList focused_keybinds(Rml::ElementDocument& document) {
			Rml::ElementList keybinds;
			Rml::Context* context = document.GetContext();
			Rml::Element* focused = context ? context->GetFocusElement() : nullptr;
			if (focused) {
				collect_focused_keybinds(*containing_input_scope(document, *focused), keybinds);
			}
			return keybinds;
		}

		string strip_inline_scripts(string_view source) {
			constexpr string_view open = "<script type=\"text/lua\"";
			constexpr string_view close = "</script>";
			string out;
			size_t offset = 0;
			while (true) {
				size_t begin = source.find(open, offset);
				if (begin == string_view::npos) {
					out += source.substr(offset);
					return out;
				}
				out += source.substr(offset, begin - offset);
				size_t end = source.find(close, begin + open.size());
				if (end == string_view::npos) {
					return out;
				}
				offset = end + close.size();
			}
		}

		string load_core_setting(string_view name, string_view fallback = {}) {
			auto value = LoadSetting("core", name, object(fallback));
			if (!value) {
				log::Warning("{}", format("[settings] {}", value.error().message));
				return string(fallback);
			}
			return value->as<string>();
		}

		f32 load_core_setting_f32(string_view name, f32 fallback) {
			auto value = LoadSetting("core", name, object(static_cast<f64>(fallback)));
			if (!value) {
				log::Warning("{}", format("[settings] {}", value.error().message));
				return fallback;
			}
			return value->as<f32>();
		}

		bool load_core_setting_bool(string_view name, bool fallback) {
			auto value = LoadSetting("core", name, object(fallback));
			if (!value) {
				log::Warning("{}", format("[settings] {}", value.error().message));
				return fallback;
			}
			return value->as<bool>();
		}

		string script_attribute(string_view tag, string_view name) {
			string_view needle = name;
			size_t offset = tag.find(needle);
			while (offset != string_view::npos) {
				bool valid_prefix = offset == 0 || std::isspace(static_cast<unsigned char>(tag[offset - 1]));
				size_t cursor = offset + needle.size();
				if (valid_prefix) {
					while (cursor < tag.size() && std::isspace(static_cast<unsigned char>(tag[cursor]))) {
						++cursor;
					}
					if (cursor < tag.size() && tag[cursor] == '=') {
						++cursor;
						while (cursor < tag.size() && std::isspace(static_cast<unsigned char>(tag[cursor]))) {
							++cursor;
						}
						if (cursor < tag.size() && (tag[cursor] == '"' || tag[cursor] == '\'')) {
							char quote = tag[cursor++];
							size_t end = tag.find(quote, cursor);
							if (end != string_view::npos) {
								return string(tag.substr(cursor, end - cursor));
							}
						}
					}
				}
				offset = tag.find(needle, offset + needle.size());
			}
			return {};
		}

	} // namespace

	class Scene::ScriptEventListener final : public Rml::EventListener {
	  public:
		explicit ScriptEventListener(Scene& scene) : scene(scene) {}

		void ProcessEvent(Rml::Event& event) override {
			string attribute;
			if (event == "click") {
				attribute = "click";
			} else if (event == "change") {
				attribute = "change";
			} else if (event == "mousedown") {
				attribute = "mousedown";
			} else if (event == "mousemove") {
				attribute = "mousemove";
			} else if (event == "mouseup") {
				attribute = "mouseup";
			} else if (event == "window-close") {
				attribute = "window-close";
			} else if (event == "window-resize") {
				attribute = "window-resize";
			}
			if (attribute.empty()) {
				return;
			}

			scene.last_mouse_position = {
				event.GetParameter<f32>("mouse_x", 0.0f),
				event.GetParameter<f32>("mouse_y", 0.0f),
			};
			scene.has_mouse_position = true;

			if (attribute == "mousedown") {
				scene.update_cursor(event.GetTargetElement(), true);
			} else if (attribute == "mouseup") {
				scene.update_cursor(event.GetTargetElement(), false, true);
			} else if (attribute == "mousemove") {
				scene.update_cursor(event.GetTargetElement(), false);
			}

			for (Rml::Element* element = event.GetTargetElement(); element; element = element->GetParentNode()) {
				string script = element->GetAttribute<Rml::String>(attribute, "");
				if (script.empty()) {
					continue;
				}

				string event_script = lua_event_table(attribute, {
					{ "mouse_x", lua_event_number(event.GetParameter<f32>("mouse_x", 0.0f)) },
					{ "mouse_y", lua_event_number(event.GetParameter<f32>("mouse_y", 0.0f)) },
					{ "button", lua_event_number(event.GetParameter<int>("button", -1)) },
				});
				event_script += script;
				scene.pending_scripts.push_back({ "event", std::move(event_script) });
				return;
			}
		}

	  private:
		Scene& scene;
	};

	Scene::Scene(
		Rml::Context& context,
		view<window> display,
		ApplicationStats& stats,
		string_view initial,
		string_view args_yaml,
		std::vector<PersistentElement> persistent_elements,
		std::vector<ScriptInstaller> script_installers,
		std::vector<FixedUpdater> fixed_updaters
	)
		: display(display),
		  stats(stats),
		  scene_args_yaml(args_yaml),
		  script_installers(std::move(script_installers)),
		  fixed_updaters(std::move(fixed_updaters)) {
		lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
		lua["scene_args_yaml"] = string(this->scene_args_yaml);

		lua.set_function("print", [](sol::variadic_args args) {
			string message;
			for (const sol::object& arg : args) {
				if (!message.empty()) {
					message += "\t";
				}
				message += lua_value_to_string(arg);
			}
			log::Info("{}", message);
		});

		string document_source = install_window_defaults(strip_inline_scripts(initial));
		document = context.LoadDocumentFromMemory(Rml::String(document_source));
		if (!document) {
			throw runtime_exception("failed to load RML document from scene source");
		}
		restore_persistent_elements(std::move(persistent_elements));
		refresh_title();

		lua.set_function("input_value", [this](string_view id) -> string {
			Rml::Element* element = find_element(id);
			if (!element) {
				return {};
			}

			if (auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(element)) {
				return string(input->GetValue());
			}
			return {};
		});

		lua.set_function("ui_value", [this](string_view id) -> string {
			Rml::Element* element = find_element(id);
			if (!element) {
				return {};
			}
			if (auto* control = rmlui_dynamic_cast<Rml::ElementFormControl*>(element)) {
				return string(control->GetValue());
			}
			return string(element->GetInnerRML());
		});

		lua.set_function("ui_blur", [this](string_view id) -> bool {
			Rml::Element* element = find_element(id);
			if (!element) {
				return false;
			}
			element->Blur();
			return true;
		});

		lua.set_function("ui_click", [this](string_view id) -> bool {
			Rml::Element* element = require_element(id, "ui_click");
			if (!element) {
				return false;
			}
			return run_element_script_event(element, "click", mouse_parameters(*element, -1.0f, -1.0f, 0), "ui-click");
		});

		lua.set_function("ui_click_selector", [this](string_view selector, sol::optional<i32> index) -> bool {
			Rml::Element* element = find_selector(selector, index.value_or(1));
			if (!element) {
				log::Error("{}", format("[ui] missing selector '{}' at {}", selector, index.value_or(1)));
				return false;
			}
			return run_element_script_event(element, "click", mouse_parameters(*element, -1.0f, -1.0f, 0), "ui-click-selector");
		});

		lua.set_function("ui_event", [this](string_view id, string_view event_name, sol::optional<sol::table> table) -> bool {
			return run_element_script_event(id, event_name, lua_event_parameters(table), "ui-event");
		});

		lua.set_function("ui_event_selector", [this](string_view selector, string_view event_name, sol::optional<i32> index, sol::optional<sol::table> table) -> bool {
			Rml::Element* element = find_selector(selector, index.value_or(1));
			if (!element) {
				log::Error("{}", format("[ui] missing selector '{}' at {}", selector, index.value_or(1)));
				return false;
			}
			return run_element_script_event(element, event_name, lua_event_parameters(table), "ui-event-selector");
		});

		lua.set_function("ui_dispatch", [this](string_view id, string_view event_name, sol::optional<sol::table> table) -> bool {
			Rml::Element* element = require_element(id, "ui_dispatch");
			if (!element) {
				return false;
			}
			return element->DispatchEvent(Rml::String(event_name), lua_event_parameters(table));
		});

		lua.set_function("ui_set_value", [this](string_view id, string_view value, sol::optional<bool> fire_change) -> bool {
			Rml::ElementFormControl* control = require_form_control(id, "ui_set_value");
			if (!control) {
				return false;
			}

			Rml::Element* element = control;
			string filtered = filter_text_for_element(*element, value);
			control->SetValue(Rml::String(filtered));
			element->SetAttribute("value", Rml::String(filtered));
			if (fire_change.value_or(true) && element_has_script_event(element, "change")) {
				run_element_script_event(id, "change", Rml::Dictionary(), "ui-set-value");
			}
			return true;
		});

		lua.set_function("ui_type", [this](string_view id, string_view text, sol::optional<bool> replace, sol::optional<bool> fire_change) -> bool {
			Rml::ElementFormControl* control = require_form_control(id, "ui_type");
			if (!control) {
				return false;
			}

			Rml::Element* element = control;
			string next_value;
			if (!replace.value_or(true)) {
				next_value = string(control->GetValue());
			}
			next_value += filter_text_for_element(*element, text);
			control->SetValue(Rml::String(next_value));
			element->SetAttribute("value", Rml::String(next_value));
			element->Focus(true);
			if (fire_change.value_or(true) && element_has_script_event(element, "change")) {
				run_element_script_event(id, "change", Rml::Dictionary(), "ui-type");
			}
			return true;
		});

		lua.set_function("ui_slider", [this](string_view id, f32 value) -> bool {
			Rml::Element* element = require_element(id, "ui_slider");
			if (!element) {
				return false;
			}

			value = std::clamp(value, 0.0f, 1.0f);
			const f32 x = element->GetAbsoluteLeft() + element->GetOffsetWidth() * value;
			const f32 y = element->GetAbsoluteTop() + element->GetOffsetHeight() * 0.5f;
			Rml::Dictionary parameters = mouse_parameters(*element, x, y, 0);
			bool began = run_element_script_event(id, "mousedown", parameters, "ui-slider");
			if (element_has_script_event(element, "mouseup")) {
				run_element_script_event(id, "mouseup", parameters, "ui-slider");
			}
			return began;
		});

		lua.set_function("ui_info", [this](string_view id) {
			sol::table info = lua.create_table();
			Rml::Element* element = find_element(id);
			info["exists"] = element != nullptr;
			if (!element) {
				return info;
			}

			info["id"] = string(element->GetId());
			info["tag"] = string(element->GetTagName());
			info["class"] = string(element->GetClassNames());
			info["left"] = element->GetAbsoluteLeft();
			info["top"] = element->GetAbsoluteTop();
			info["width"] = element->GetOffsetWidth();
			info["height"] = element->GetOffsetHeight();
			info["children"] = element->GetNumChildren();
			if (auto* control = rmlui_dynamic_cast<Rml::ElementFormControl*>(element)) {
				info["value"] = string(control->GetValue());
			}
			return info;
		});

		lua.set_function("ui_find", [this](string_view selector) {
			sol::table list = lua.create_table();
			Rml::ElementList elements;
			document->QuerySelectorAll(elements, Rml::String(selector));
			i32 index = 1;
			for (Rml::Element* element : elements) {
				sol::table item = lua.create_table();
				item["id"] = string(element->GetId());
				item["tag"] = string(element->GetTagName());
				item["class"] = string(element->GetClassNames());
				item["left"] = element->GetAbsoluteLeft();
				item["top"] = element->GetAbsoluteTop();
				item["width"] = element->GetOffsetWidth();
				item["height"] = element->GetOffsetHeight();
				list[index++] = item;
			}
			return list;
		});

		lua.set_function("ui_dump", [this](sol::optional<string_view> id, sol::optional<i32> depth) -> bool {
			Rml::Element* root = id ? find_element(*id) : document;
			if (!root) {
				log::Error("{}", format("[ui] missing element '{}'", id ? string(*id) : string("<document>")));
				return false;
			}

			const i32 max_depth = std::max(0, depth.value_or(2));
			std::function<void(Rml::Element*, i32)> dump = [&](Rml::Element* element, i32 level) {
				if (!element || level > max_depth) {
					return;
				}
				string indent(static_cast<size_t>(level) * 2, ' ');
				string element_id = string(element->GetId());
				string class_names = string(element->GetClassNames());
				string id_text = element_id.empty() ? "" : format("#{}", element_id);
				string class_text = class_names.empty() ? "" : format(".{}", class_names);
				log::Info("{}", format("[ui] {}{}{}{} {}x{} at {},{}",
								indent,
								string(element->GetTagName()),
								id_text,
								class_text,
								element->GetOffsetWidth(),
								element->GetOffsetHeight(),
								element->GetAbsoluteLeft(),
								element->GetAbsoluteTop()));
				for (int i = 0; i < element->GetNumChildren(); ++i) {
					dump(element->GetChild(i), level + 1);
				}
			};
			dump(root, 0);
			return true;
		});

		lua.set_function("set_document_title", [this](string_view title) {
			document->SetTitle(Rml::String(title));
			refresh_title();
		});

		sol::table game = lua.create_table();
			lua.globals()["game"] = game;
			lua["_G"]["game"] = game;
			game["load"] = [this](sol::object, string_view path, sol::optional<string_view> args_yaml) -> bool {
				pending_load_request = LoadRequest{ string(path), string(args_yaml.value_or("")) };
				return true;
		};
		game["exit"] = [this](sol::object) {
			if (this->display) {
				Window::SetShouldClose(this->display, true);
			}
		};
			game["scene"] = [this](sol::this_state state, sol::object) {
			sol::state_view lua(state);
			sol::table scene = lua.create_table();
			scene["set_rml"] = [this](sol::object, string_view id, string_view rml) {
				set_rml(id, rml);
			};
			scene["set_text"] = [this](sol::object, string_view id, string_view text) {
				if (Rml::Element* element = document->GetElementById(Rml::String(id))) {
					Rml::String next(rml_escape(text));
					if (element->GetInnerRML() != next) {
						element->SetInnerRML(next);
					}
				}
			};
			scene["set_property"] = [this](sol::object, string_view id, string_view name, string_view value) {
				if (Rml::Element* element = find_element(id)) {
					element->SetProperty(Rml::String(name), Rml::String(value));
				}
			};
			scene["set_attribute"] = [this](sol::object, string_view id, string_view name, sol::object value) {
				Rml::Element* element = find_element(id);
				if (element) {
					if (value.is<f32>()) {
						set_attribute(id, name, value.as<f32>());
					} else if (value.is<f64>()) {
						set_attribute(id, name, static_cast<f32>(value.as<f64>()));
					} else {
						set_attribute(id, name, lua_value_to_string(value));
					}
				}
			};
			scene["focus"] = [this](sol::object, string_view id) -> bool {
				Rml::Element* element = find_element(id);
				return element ? element->Focus(true) : false;
			};
			scene["focused"] = [this](sol::object) -> string {
				if (!document) {
					return {};
				}
				Rml::Context* context = document->GetContext();
				Rml::Element* focused = context ? context->GetFocusElement() : nullptr;
				return focused ? string(focused->GetId()) : string();
			};
			scene["remove_element"] = [this](sol::object, string_view id) {
				Rml::Element* element = find_element(id);
				if (!element) {
					return;
				}
				if (Rml::Element* parent = element->GetParentNode()) {
					parent->RemoveChild(element);
				}
			};
			scene["element_exists"] = [this](sol::object, string_view tag_or_id, sol::optional<string_view> maybe_id) -> bool {
				Rml::Element* element = document->GetElementById(Rml::String(maybe_id.value_or(tag_or_id)));
				if (!element) {
					return false;
				}
				if (!maybe_id) {
					return true;
				}
				return element->GetTagName() == Rml::String(tag_or_id);
			};
			scene["element_left"] = [this](sol::object, string_view id) -> f32 {
				if (Rml::Element* element = find_element(id)) {
					return element->GetAbsoluteLeft();
				}
				return 0.0f;
			};
			scene["element_top"] = [this](sol::object, string_view id) -> f32 {
				if (Rml::Element* element = find_element(id)) {
					return element->GetAbsoluteTop();
				}
				return 0.0f;
			};
			scene["element_width"] = [this](sol::object, string_view id) -> f32 {
				if (Rml::Element* element = find_element(id)) {
					return element->GetOffsetWidth();
				}
				return 0.0f;
			};
			scene["element_height"] = [this](sol::object, string_view id) -> f32 {
				if (Rml::Element* element = find_element(id)) {
					return element->GetOffsetHeight();
				}
				return 0.0f;
			};
			scene["context_width"] = [this](sol::object) -> i32 {
				return document->GetContext()->GetDimensions().x;
			};
			scene["context_height"] = [this](sol::object) -> i32 {
				return document->GetContext()->GetDimensions().y;
			};
			return scene;
		};

		lua.set_function("rml_escape", [](string_view value) {
			return rml_escape(value);
		});

		sol::table sound_type = lua.create_table();
		sound_type["ui"] = lua.create_table_with("id", "ui");
		sound_type["effects"] = lua.create_table_with("id", "effects");
		sound_type["music"] = lua.create_table_with("id", "music");
		sound_type["master"] = lua.create_table_with("id", "master");
		lua["sound_type"] = sound_type;
		InstallSoundScript(lua);
		InstallCursorScript(lua, display);
		InstallPrototypeInspectorScript(lua);

		lua.set_function("localize", [](string_view section, string_view key, sol::variadic_args args) {
			vector<string> parameters;
			for (const sol::object& arg : args) {
				parameters.push_back(lua_value_to_string(arg));
			}
			return Localize(section, key, span<const string>(parameters.data(), parameters.size()));
		});

		lua.set_function("loaded_language", []() {
			return string(LoadedLanguage());
		});

		lua.set_function("available_language_count", []() -> i32 {
			return static_cast<i32>(AvailableLanguages().size());
		});

		lua.set_function("available_languages", [this]() {
			sol::table list = lua.create_table();
			i32 index = 1;
			for (const LanguageInfo& language : AvailableLanguages()) {
				sol::table item = lua.create_table();
				item["id"] = language.id;
				item["name"] = language.name;
				item["native_name"] = language.native_name;
				list[index++] = item;
			}
			return list;
		});

		lua.set_function("available_language_id", [](i32 index) {
			span<const LanguageInfo> languages = AvailableLanguages();
			if (index < 1 || static_cast<size_t>(index) > languages.size()) {
				return string();
			}
			return languages[static_cast<size_t>(index - 1)].id;
		});

		lua.set_function("available_language_name", [](i32 index) {
			span<const LanguageInfo> languages = AvailableLanguages();
			if (index < 1 || static_cast<size_t>(index) > languages.size()) {
				return string();
			}
			return languages[static_cast<size_t>(index - 1)].name;
		});

		lua.set_function("available_language_native_name", [](i32 index) {
			span<const LanguageInfo> languages = AvailableLanguages();
			if (index < 1 || static_cast<size_t>(index) > languages.size()) {
				return string();
			}
			return languages[static_cast<size_t>(index - 1)].native_name;
		});

		lua.set_function("loaded_language_native_name", []() {
			return string(AvailableLanguageNativeName(LoadedLanguage()));
		});

		lua.set_function("set_language", [](string_view language) -> bool {
			if (error err = SetLanguage(language)) {
				log::Error("{}", format("[settings] {}", err.message));
				return false;
			}
			return true;
		});

		lua.set_function("settings_master_volume", []() -> f32 {
			return load_core_setting_f32("sound.master", 1.0f);
		});

		lua.set_function("settings_music_volume", []() -> f32 {
			return load_core_setting_f32("sound.music", 0.8f);
		});

		lua.set_function("settings_effects_volume", []() -> f32 {
			return load_core_setting_f32("sound.effects", 1.0f);
		});

		lua.set_function("settings_fullscreen", []() -> bool {
			return load_core_setting_bool("graphics.fullscreen", false);
		});

		lua.set_function("settings_vsync", []() -> bool {
			return load_core_setting_bool("graphics.vsync", false);
		});

		lua.set_function("settings", [](sol::this_state state) -> sol::table {
			sol::state_view lua(state);

			sol::table sound = lua.create_table();
			sound["master"] = load_core_setting_f32("sound.master", 1.0f);
			sound["music"] = load_core_setting_f32("sound.music", 0.8f);
			sound["effects"] = load_core_setting_f32("sound.effects", 1.0f);

			sol::table graphics = lua.create_table();
			graphics["fullscreen"] = load_core_setting_bool("graphics.fullscreen", false);
			graphics["vsync"] = load_core_setting_bool("graphics.vsync", false);
			graphics["max_fps"] = load_core_setting_f32("graphics.max-fps", 60.0f);

			sol::table result = lua.create_table();
			result["language"] = load_core_setting("language", default_language);
			result["sound"] = sound;
			result["graphics"] = graphics;
			return result;
		});

		lua.set_function("settings_max_fps", [this]() -> f32 {
			return this->stats.max_fps.load(std::memory_order_relaxed);
		});

		lua.set_function("graphics_backend", []() -> string_view {
			return graphics_backend_name();
		});

		lua.set_function("include", [this](string_view path) {
			string include_path(path);
			report<string> script = ReadVirtualTextFile(include_path);
			if (!script) {
				throw runtime_exception(format("include '{}': {}", include_path, script.error().message));
			}
			if (!execute_script(*script, include_path)) {
				throw runtime_exception(format("include '{}' failed", include_path));
			}
		});

		lua.set_function("set_max_fps", [this](f32 value) {
			this->stats.max_fps.store(value, std::memory_order_relaxed);
		});

		lua.set_function("current_fps", [this]() -> f32 {
			return this->stats.current_fps.load(std::memory_order_relaxed);
		});

		lua.set_function("current_ups", [this]() -> f32 {
			return this->stats.current_ups.load(std::memory_order_relaxed);
		});

		lua.set_function("render_profile", [this](bool enabled) {
			this->stats.render_profile_enabled.store(enabled, std::memory_order_relaxed);
		});

		lua.set_function("profile", [](bool enabled) {
			SetProfilerEnabled(enabled);
		});

		lua.set_function("profile_reset", []() {
			ResetProfiler();
		});

		lua.set_function("profile_log", [](sol::optional<string_view> label) {
			LogProfileSnapshot(label.value_or("profile"));
		});

		lua.set_function("fullscreen", [this]() -> bool {
			return this->display ? Window::Fullscreen(this->display) : false;
		});

		lua.set_function("set_fullscreen", [this](bool fullscreen) {
			if (this->display) {
				Window::RequestFullscreen(this->display, fullscreen);
			}
		});

		lua.set_function("save_settings", [this](f32 master, f32 music, f32 effects, bool fullscreen, bool vsync, sol::object max_fps_value) -> bool {
			f32 max_fps = load_core_setting_f32("graphics.max-fps", 60.0f);
			if (max_fps_value.is<f32>()) {
				max_fps = max_fps_value.as<f32>();
			} else if (max_fps_value.is<f64>()) {
				max_fps = static_cast<f32>(max_fps_value.as<f64>());
			} else if (max_fps_value.is<i32>()) {
				max_fps = static_cast<f32>(max_fps_value.as<i32>());
			}
			bool fullscreen_changed = load_core_setting_bool("graphics.fullscreen", false) != fullscreen;
			if (error err = SaveSetting("core", "sound.master", object(static_cast<f64>(master)))) {
				log::Error("{}", format("[settings] {}", err.message));
				return false;
			}
			if (error err = SaveSetting("core", "sound.music", object(static_cast<f64>(music)))) {
				log::Error("{}", format("[settings] {}", err.message));
				return false;
			}
			if (error err = SaveSetting("core", "sound.effects", object(static_cast<f64>(effects)))) {
				log::Error("{}", format("[settings] {}", err.message));
				return false;
			}
			if (error err = SaveSetting("core", "graphics.fullscreen", object(fullscreen))) {
				log::Error("{}", format("[settings] {}", err.message));
				return false;
			}
			if (error err = SaveSetting("core", "graphics.vsync", object(vsync))) {
				log::Error("{}", format("[settings] {}", err.message));
				return false;
			}
			if (error err = SaveSetting("core", "graphics.max-fps", object(static_cast<f64>(max_fps)))) {
				log::Error("{}", format("[settings] {}", err.message));
				return false;
			}
			this->stats.max_fps.store(max_fps, std::memory_order_relaxed);
			if (this->display) {
				Window::SetVsync(this->display, vsync);
			}
			if (fullscreen_changed && this->display) {
				Window::RequestFullscreen(this->display, fullscreen);
			}
			return true;
		});

		for (const ScriptInstaller& installer : this->script_installers) {
			installer(lua, *document);
		}
		install_ui_automation_helpers();

		script_events = std::make_unique<ScriptEventListener>(*this);
		run_inline_scripts(initial);
		bind_script_events();
		document->Show();
	}

	Scene::~Scene() {
		SetCursorPrototype(display, {});
		if (document) {
			Rml::Context* context = document->GetContext();
			ReleaseWindowDocumentEvents(*document);
			clear_script_events();
			context->UnloadDocument(document);
			document = nullptr;
		}
		pending_scripts.clear();
		lua = sol::state();
	}

	std::vector<Scene::PersistentElement> Scene::release_persistent_elements() {
		std::vector<PersistentElement> persistent_elements;
		if (!document) {
			return persistent_elements;
		}

		clear_script_events();
		Rml::ElementList elements;
		document->QuerySelectorAll(elements, "*");
		for (Rml::Element* element : elements) {
			if (!element || !element_persists_for_scene(*element) || element->GetId().empty()) {
				continue;
			}
			Rml::Element* parent = element->GetParentNode();
			if (!parent) {
				continue;
			}
			Rml::ElementPtr released = parent->RemoveChild(element);
			if (released) {
				persistent_elements.push_back({ string(released->GetId()), std::move(released) });
			}
		}
		return persistent_elements;
	}

	void Scene::restore_persistent_elements(std::vector<PersistentElement> persistent_elements) {
		if (!document) {
			return;
		}

		for (PersistentElement& persistent : persistent_elements) {
			if (persistent.id.empty() || !persistent.element) {
				continue;
			}

			Rml::Element* placeholder = document->GetElementById(Rml::String(persistent.id));
			Rml::Element* parent = placeholder ? placeholder->GetParentNode() : nullptr;
			if (parent) {
				copy_persistent_placeholder_attributes(*persistent.element, *placeholder);
				parent->ReplaceChild(std::move(persistent.element), placeholder);
			}
		}
	}

	void Scene::clear_script_events() {
		if (!script_events || !script_events_bound || !document) {
			return;
		}
		document->RemoveEventListener("click", script_events.get());
		document->RemoveEventListener("change", script_events.get());
		document->RemoveEventListener("mousedown", script_events.get());
		document->RemoveEventListener("mousemove", script_events.get());
		document->RemoveEventListener("mouseup", script_events.get());
		document->RemoveEventListener("window-close", script_events.get());
		document->RemoveEventListener("window-resize", script_events.get());
		script_events_bound = false;
	}

	void Scene::bind_script_events() {
		if (!script_events || script_events_bound || !document) {
			return;
		}
		document->AddEventListener("click", script_events.get());
		document->AddEventListener("change", script_events.get());
		document->AddEventListener("mousedown", script_events.get());
		document->AddEventListener("mousemove", script_events.get());
		document->AddEventListener("mouseup", script_events.get());
		document->AddEventListener("window-close", script_events.get());
		document->AddEventListener("window-resize", script_events.get());
		script_events_bound = true;
	}

	void Scene::run_inline_scripts(string_view source) {
		constexpr string_view open = "<script type=\"text/lua\"";
		constexpr string_view close = "</script>";
		size_t offset = 0;
		while (true) {
			size_t begin = source.find(open, offset);
			if (begin == string_view::npos) {
				return;
			}
			size_t tag_end = source.find('>', begin + open.size());
			if (tag_end == string_view::npos) {
				return;
			}
			string_view tag = source.substr(begin, tag_end - begin + 1);
			size_t body_begin = tag_end + 1;
			size_t end = source.find(close, body_begin);
			if (end == string_view::npos) {
				return;
			}

			string src = script_attribute(tag, "src");
			if (!src.empty()) {
				report<string> script = ReadVirtualTextFile(src);
				if (!script) {
					log::Error("{}", format("[scene] {}: {}", src, script.error().message));
				} else {
					run_script(*script, src);
				}
			} else {
				run_script(source.substr(body_begin, end - body_begin), "inline script");
			}
			offset = end + close.size();
		}
	}

	void Scene::run_script(string_view script, string_view source_name) {
		execute_script(script, source_name);
	}

	bool Scene::execute_script(string_view script, string_view source_name) {
		sol::protected_function_result result = lua.safe_script(string(script), sol::script_pass_on_error);
		if (!result.valid()) {
			sol::error error = result;
			log::Error("{}", format("[scene] {}: {}", source_name, error.what()));
			return false;
		}
		return true;
	}

	bool Scene::run_pending_scripts() {
		std::vector<PendingScript> scripts;
		scripts.swap(pending_scripts);

		bool ran_scripts = false;
		for (const PendingScript& pending : scripts) {
			ran_scripts = true;
			execute_script(pending.script, pending.source_name);
		}

		if (ran_scripts) {
			update_cursor_at_last_mouse();
		}
		return ran_scripts;
	}

	void Scene::install_ui_automation_helpers() {
		constexpr string_view helpers = R"lua(
__leaf_ui_automation = {
	queue = {},
	next_id = 1,
	last_error = "",
}

local function __leaf_compile_automation(value, label)
	if type(value) == "function" then
		return value
	end
	if type(value) == "string" then
		local fn, err = load(value, label or "ui-automation", "t", _G)
		if fn == nil then
			__leaf_ui_automation.last_error = tostring(err)
			print("[ui-wait] " .. tostring(err))
			return nil
		end
		return fn
	end
	__leaf_ui_automation.last_error = "expected function or string"
	print("[ui-wait] expected function or string")
	return nil
end

local function __leaf_schedule_wait(condition, action, timeout_seconds, label)
	local condition_fn = __leaf_compile_automation(condition, label or "ui-wait-condition")
	local action_fn = __leaf_compile_automation(action, label or "ui-wait-action")
	if condition_fn == nil or action_fn == nil then
		return 0
	end

	local id = __leaf_ui_automation.next_id
	__leaf_ui_automation.next_id = id + 1
	__leaf_ui_automation.queue[#__leaf_ui_automation.queue + 1] = {
		id = id,
		condition = condition_fn,
		action = action_fn,
		start = __leaf_ui_automation.now or 0,
		timeout = timeout_seconds,
		label = label or "ui-wait",
	}
	return id
end

function ui_wait_until(condition, action, timeout_seconds)
	return __leaf_schedule_wait(condition, action, timeout_seconds, "ui-wait-until")
end

function ui_wait_element(id, action, timeout_seconds)
	return ui_wait_until(function()
		return element_exists(id)
	end, action, timeout_seconds)
end

function ui_wait_not_element(id, action, timeout_seconds)
	return ui_wait_until(function()
		return not element_exists(id)
	end, action, timeout_seconds)
end

function ui_wait_selector(selector, action, timeout_seconds)
	return ui_wait_until(function()
		return #ui_find(selector) > 0
	end, action, timeout_seconds)
end

function ui_wait_value(id, value, action, timeout_seconds)
	return ui_wait_until(function()
		return ui_value(id) == tostring(value)
	end, action, timeout_seconds)
end

function ui_wait_frames(frames, action)
	frames = math.max(0, math.floor(tonumber(frames) or 0))
	local remaining = frames
	return ui_wait_until(function()
		remaining = remaining - 1
		return remaining <= 0
	end, action)
end

function ui_wait_seconds(seconds, action)
	local duration = math.max(0, tonumber(seconds) or 0)
	local start_time = __leaf_ui_automation.now or 0
	return ui_wait_until(function()
		return ((__leaf_ui_automation.now or 0) - start_time) >= duration
	end, action)
end

function ui_automation_pending()
	return #__leaf_ui_automation.queue
end

function ui_automation_clear()
	__leaf_ui_automation.queue = {}
end

function ui_automation_last_error()
	return __leaf_ui_automation.last_error
end

function __leaf_ui_automation_update(time_seconds)
	__leaf_ui_automation.now = time_seconds
	local next_queue = {}
	for _, wait in ipairs(__leaf_ui_automation.queue) do
		local keep = true
		local ok, ready = pcall(wait.condition)
		if not ok then
			__leaf_ui_automation.last_error = tostring(ready)
			print("[ui-wait] condition failed: " .. tostring(ready))
			keep = false
		elseif ready then
			local action_ok, action_error = pcall(wait.action)
			if not action_ok then
				__leaf_ui_automation.last_error = tostring(action_error)
				print("[ui-wait] action failed: " .. tostring(action_error))
			end
			keep = false
		elseif wait.timeout ~= nil and (time_seconds - wait.start) >= wait.timeout then
			__leaf_ui_automation.last_error = "timeout: " .. tostring(wait.label)
			print("[ui-wait] timeout: " .. tostring(wait.label))
			keep = false
		end
		if keep then
			next_queue[#next_queue + 1] = wait
		end
	end
	__leaf_ui_automation.queue = next_queue
end
)lua";
		run_script(helpers, "ui-automation-helpers");
	}

	void Scene::update_ui_automation(f64 elapsed) {
		sol::object update_object = lua["__leaf_ui_automation_update"];
		if (update_object.get_type() != sol::type::function) {
			return;
		}

		sol::protected_function update = update_object;
		sol::protected_function_result result = update(elapsed);
		if (!result.valid()) {
			sol::error error = result;
			log::Error("{}", format("[scene] ui-automation: {}", error.what()));
		}
	}

	void Scene::update_cursor(Rml::Element* start, bool pressed, bool released) {
		if (released) {
			active_pressed_cursor.clear();
		} else if (!active_pressed_cursor.empty()) {
			SetCursorPrototype(display, active_pressed_cursor);
			return;
		}

		for (Rml::Element* element = start; element; element = element->GetParentNode()) {
			string cursor;
			if (pressed) {
				cursor = element->GetAttribute<Rml::String>("pressed-cursor", "");
				if (cursor.empty()) {
					cursor = element->GetAttribute<Rml::String>("click-cursor", "");
				}
			}
			if (cursor.empty()) {
				cursor = element->GetAttribute<Rml::String>("hover-cursor", "");
			}
			if (cursor.empty()) {
				cursor = element->GetAttribute<Rml::String>("cursor", "");
			}
			if (!cursor.empty()) {
				if (pressed) {
					active_pressed_cursor = cursor;
				}
				SetCursorPrototype(display, cursor);
				return;
			}
		}
		if (pressed) {
			active_pressed_cursor.clear();
		}
		SetCursorPrototype(display, {});
	}

	void Scene::update_cursor_at_last_mouse() {
		if (!document || !has_mouse_position) {
			return;
		}
		Rml::Context* context = document->GetContext();
		if (!context) {
			return;
		}
		update_cursor(context->GetElementAtPoint(last_mouse_position), false);
	}

	Rml::Element* Scene::find_element(string_view id) const {
		return document ? document->GetElementById(Rml::String(id)) : nullptr;
	}

	Rml::Element* Scene::find_selector(string_view selector, i32 index) const {
		if (!document || index < 1) {
			return nullptr;
		}

		Rml::ElementList elements;
		document->QuerySelectorAll(elements, Rml::String(selector));
		const size_t offset = static_cast<size_t>(index - 1);
		if (offset >= elements.size()) {
			return nullptr;
		}
		return elements[offset];
	}

	Rml::Element* Scene::require_element(string_view id, string_view api_name) const {
		Rml::Element* element = find_element(id);
		if (!element) {
			log::Error("{}", format("[ui] {} missing element '{}'", api_name, id));
		}
		return element;
	}

	Rml::ElementFormControl* Scene::require_form_control(string_view id, string_view api_name) const {
		Rml::Element* element = require_element(id, api_name);
		if (!element) {
			return nullptr;
		}

		auto* control = rmlui_dynamic_cast<Rml::ElementFormControl*>(element);
		if (!control) {
			log::Error("{}", format("[ui] {} element '{}' is not a form control", api_name, id));
		}
		return control;
	}

	bool Scene::run_element_script_event(Rml::Element* start, string_view attribute, const Rml::Dictionary& parameters, string_view source_name) {
		if (!start) {
			log::Error("[ui] missing element");
			return false;
		}

		for (Rml::Element* element = start; element; element = element->GetParentNode()) {
			string script = element->GetAttribute<Rml::String>(Rml::String(attribute), "");
			if (script.empty()) {
				continue;
			}

			auto mouse_x = parameters.find("mouse_x");
			auto mouse_y = parameters.find("mouse_y");
			auto button = parameters.find("button");
			string event_script = lua_event_table(attribute, {
				{ "mouse_x", lua_event_number(mouse_x != parameters.end() ? mouse_x->second.Get<f32>(0.0f) : 0.0f) },
				{ "mouse_y", lua_event_number(mouse_y != parameters.end() ? mouse_y->second.Get<f32>(0.0f) : 0.0f) },
				{ "button", lua_event_number(button != parameters.end() ? button->second.Get<i32>(-1) : -1) },
			});
			event_script += script;
			run_script(event_script, source_name);
			return true;
		}

		log::Error("{}", format("[ui] element has no '{}' script", attribute));
		return false;
	}

	bool Scene::run_element_script_event(string_view id, string_view attribute, const Rml::Dictionary& parameters, string_view source_name) {
		Rml::Element* start = find_element(id);
		if (!start) {
			log::Error("{}", format("[ui] missing element '{}'", id));
			return false;
		}
		return run_element_script_event(start, attribute, parameters, source_name);
	}

	void Scene::update() {
		if (run_pending_scripts() && pending_load_request) {
			return;
		}

		auto now = std::chrono::steady_clock::now();
		f64 elapsed = std::chrono::duration<f64>(now - start_time).count();
		update_ui_automation(elapsed);

		sol::object update_object = lua["update"];
		if (update_object.get_type() != sol::type::function) {
			return;
		}

		sol::protected_function update = update_object;
		sol::protected_function_result result = update(elapsed);
		if (!result.valid()) {
			sol::error error = result;
			log::Error("{}", format("[scene] {}", error.what()));
		}
	}

	void Scene::input(const input_event& event) {
		if (!document) {
			return;
		}

		if (event.type == INPUT_EVENT_SCROLL) {
			last_mouse_position = { event.position.x, event.position.y };
			has_mouse_position = true;
			Rml::Context* context = document->GetContext();
			if (!context) {
				return;
			}

			Rml::Element* start = context->GetElementAtPoint({ event.position.x, event.position.y });
			update_cursor(start, false);
			for (Rml::Element* element = start; element; element = element->GetParentNode()) {
				string script = element->GetAttribute<Rml::String>("mousescroll", "");
				if (script.empty()) {
					continue;
				}

				string event_script = lua_event_table("mousescroll", {
					{ "mouse_x", lua_event_number(event.position.x) },
					{ "mouse_y", lua_event_number(event.position.y) },
					{ "scroll_x", lua_event_number(event.delta.x) },
					{ "scroll_y", lua_event_number(event.delta.y) },
				});
				event_script += script;
				pending_scripts.push_back({ "input-scroll", std::move(event_script) });
				return;
			}
			return;
		}

		if (event.type == INPUT_EVENT_POINTER_MOVE || event.type == INPUT_EVENT_CONTROL) {
			last_mouse_position = { event.position.x, event.position.y };
			has_mouse_position = true;
		}

		string attribute;
		string event_key;
		string event_action;
		u32 event_character = 0;

		if (event.type == INPUT_EVENT_TEXT) {
			attribute = "keydown";
			event_character = event.character;
		} else if (event.type == INPUT_EVENT_CONTROL && event.control.type == INPUT_CONTROL_KEY) {
			if (event.state == input_state::Pressed) {
				attribute = "keydown";
			} else if (event.state == input_state::Released) {
				attribute = "keyup";
			}
			event_key = key_name(static_cast<input_key>(event.control.value));
		}

		if (attribute.empty()) {
			return;
		}

		Rml::Context* context = document->GetContext();
		Rml::Element* focused = context ? context->GetFocusElement() : nullptr;
		if (focused && !event_key.empty()) {
			string focused_script = focused->GetAttribute<Rml::String>(Rml::String(format("{}-{}", attribute, event_key)), "");
			if (!focused_script.empty()) {
				string event_script = lua_event_table(attribute, {
					{ "key", lua_event_string(event_key) },
					{ "character", lua_event_number(static_cast<i32>(event_character)) },
					{ "modifiers", lua_event_number(static_cast<i32>(event.modifiers.value)) },
				});
				event_script += focused_script;
				pending_scripts.push_back({ "input", std::move(event_script) });
			}
		}

		Rml::ElementList keybinds = focused_keybinds(*document);
		for (Rml::Element* keybind : keybinds) {
			if (!keybind) {
				continue;
			}

			bool matched = false;
			event_action.clear();
			if (event.type == INPUT_EVENT_TEXT) {
				matched = keybind_matches_action_name(*keybind, text_key_name(event.character), event_action);
				if (!matched) {
					matched = keybind_matches_text(*keybind, event.character);
				}
				if (matched && event_key.empty()) {
					event_key = text_key_name(event.character);
				}
			} else {
				matched = keybind_matches_action(*keybind, static_cast<input_key>(event.control.value), event_action);
				if (!matched) {
					matched = keybind_matches_key(*keybind, static_cast<input_key>(event.control.value));
				}
			}
			if (!matched) {
				continue;
			}

			string script = keybind->GetAttribute<Rml::String>(attribute, "");
			if (script.empty()) {
				continue;
			}

			string event_script = lua_event_table(attribute, {
				{ "key", lua_event_string(event_key) },
				{ "character", lua_event_number(static_cast<i32>(event_character)) },
				{ "modifiers", lua_event_number(static_cast<i32>(event.modifiers.value)) },
			});
			event_script += script;
			pending_scripts.push_back({ "input", std::move(event_script) });
		}
	}

	void Scene::fixed_update() {
		if (!document) {
			return;
		}
		for (const FixedUpdater& updater : fixed_updaters) {
			updater(*document);
		}
	}

	void Scene::set_rml(string_view id, string_view rml) {
		if (Rml::Element* element = document->GetElementById(Rml::String(id))) {
			Rml::String next(rml);
			if (element->GetInnerRML() != next) {
				string focused_id;
				string focused_value;
				Rml::Context* context = document->GetContext();
				Rml::Element* focused = context ? context->GetFocusElement() : nullptr;
				if (focused && !focused->GetId().empty()) {
					focused_id = string(focused->GetId());
					if (auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(focused)) {
						focused_value = string(input->GetValue());
					}
				}
				element->SetInnerRML(next);
				if (!focused_id.empty()) {
					if (Rml::Element* restored = document->GetElementById(Rml::String(focused_id))) {
						if (auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(restored)) {
							input->SetValue(Rml::String(focused_value));
							restored->SetAttribute("value", Rml::String(focused_value));
						}
						restored->Focus(true);
					}
				}
			}
		}
	}

	void Scene::set_attribute(string_view id, string_view name, string_view value) {
		if (Rml::Element* element = document->GetElementById(Rml::String(id))) {
			Rml::String attribute_name(name);
			Rml::String next(value);
			if (element->GetAttribute<Rml::String>(attribute_name, "") != next) {
				element->SetAttribute(attribute_name, next);
			}
		}
	}

	void Scene::set_attribute(string_view id, string_view name, f32 value) {
		if (Rml::Element* element = document->GetElementById(Rml::String(id))) {
			element->SetAttribute(Rml::String(name), value);
		}
	}

	void Scene::queue_script(string script, string source_name) {
		pending_scripts.push_back({ std::move(source_name), std::move(script) });
	}

	std::optional<Scene::LoadRequest> Scene::take_load_request() {
		std::optional<LoadRequest> request = std::move(pending_load_request);
		pending_load_request.reset();
		return request;
	}

	bool Scene::has_text_input_focus() const {
		if (!document) {
			return false;
		}

		Rml::Context* context = document->GetContext();
		Rml::Element* focused = context ? context->GetFocusElement() : nullptr;
		return focused && rmlui_dynamic_cast<Rml::ElementFormControlInput*>(focused) != nullptr;
	}

	bool Scene::accepts_text_input(u32 character) const {
		if (!document) {
			return true;
		}

		Rml::Context* context = document->GetContext();
		Rml::Element* focused = context ? context->GetFocusElement() : nullptr;
		if (!focused || focused->GetAttribute<Rml::String>("input-filter", "") != "digits") {
			return true;
		}

		return character >= '0' && character <= '9';
	}

	string Scene::filter_text_input(string_view text) const {
		if (!document) {
			return string(text);
		}

		Rml::Context* context = document->GetContext();
		Rml::Element* focused = context ? context->GetFocusElement() : nullptr;
		if (!focused || focused->GetAttribute<Rml::String>("input-filter", "") != "digits") {
			return string(text);
		}

		string filtered;
		filtered.reserve(text.size());
		for (char character : text) {
			if (character >= '0' && character <= '9') {
				filtered += character;
			}
		}
		return filtered;
	}

	void Scene::refresh_title() {
		title_text = document ? string(document->GetTitle()) : string();
	}

	string_view Scene::title() const {
		return title_text;
	}

	unique_ptr<Scene> make_scene(
		Rml::Context& context,
		view<window> display,
		ApplicationStats& stats,
		string_view initial,
		string_view args_yaml,
		std::vector<Scene::PersistentElement> persistent_elements,
		std::vector<Scene::ScriptInstaller> script_installers,
		std::vector<Scene::FixedUpdater> fixed_updaters
	) {
		return make_unique<Scene>(context, display, stats, initial, args_yaml, std::move(persistent_elements), std::move(script_installers), std::move(fixed_updaters));
	}
} // namespace lf
