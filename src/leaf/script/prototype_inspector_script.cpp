#include "prototype_inspector_script.hpp"

#include <leaf/script/database.hpp>
#include <leaf/script/registry.hpp>
#include <leaf/script/texture_prototype.hpp>

namespace lf {
	static const PrototypeTypeFunctions* prototype_type(string_view type) {
		for (const PrototypeTypeFunctions& functions : PrototypeTypeRegistry::functions) {
			if (functions.type && functions.type() == type) {
				return &functions;
			}
		}
		return nullptr;
	}

	static void push_field_table(sol::state& lua, sol::table& out, size_t index, const PrototypeField& field) {
		sol::table row = lua.create_table();
		row["name"] = field.name;
		row["value"] = field.value;
		row["kind"] = field.kind;
		out[index] = row;
	}

	static sol::table field_list_table(sol::state& lua, const PrototypeFieldList& fields) {
		sol::table out = lua.create_table();
		for (size_t i = 0; i < fields.size(); ++i) {
			push_field_table(lua, out, i + 1, fields[i]);
		}
		return out;
	}

	void InstallPrototypeInspectorScript(sol::state& lua) {
		lua.set_function("prototype_types", [&lua]() {
			sol::table out = lua.create_table();
			size_t index = 1;
			for (const PrototypeTypeFunctions& functions : PrototypeTypeRegistry::functions) {
				if (functions.type) {
					out[index++] = string(functions.type());
				}
			}
			return out;
		});

		lua.set_function("prototype_names", [&lua](string_view type) {
			sol::table out = lua.create_table();
			if (const PrototypeTypeFunctions* functions = prototype_type(type)) {
				size_t count = functions->count ? functions->count() : 0;
				for (size_t i = 0; i < count; ++i) {
					out[i + 1] = functions->name ? string(functions->name(i)) : string();
				}
			}
			return out;
		});

		lua.set_function("prototype_localized_name", [](string_view type, size_t index) {
			if (const PrototypeTypeFunctions* functions = prototype_type(type)) {
				size_t count = functions->count ? functions->count() : 0;
				if (index >= 1 && index <= count && functions->runtime_fields) {
					for (const PrototypeField& field : functions->runtime_fields(index - 1)) {
						if (field.name == "localized_name") {
							return field.value;
						}
					}
				}
			}
			return string();
		});

		lua.set_function("prototype_runtime_fields", [&lua](string_view type, size_t index) {
			if (const PrototypeTypeFunctions* functions = prototype_type(type)) {
				size_t count = functions->count ? functions->count() : 0;
				if (index >= 1 && index <= count && functions->runtime_fields) {
					return field_list_table(lua, functions->runtime_fields(index - 1));
				}
			}
			return lua.create_table();
		});

		lua.set_function("texture_animation_fps", [](string_view texture_name) {
			for (const TexturePrototype& texture : Database<TexturePrototype>::prototypes) {
				if (texture.name == texture_name) {
					return texture.frames_per_second > 0.0f ? texture.frames_per_second : 1.0f;
				}
			}
			return 1.0f;
		});

		lua.set_function("texture_animation_frames", [&lua](string_view texture_name) {
			sol::table out = lua.create_table();
			for (const TexturePrototype& texture : Database<TexturePrototype>::prototypes) {
				if (texture.name != texture_name) {
					continue;
				}
				for (size_t i = 0; i < texture.frames.size(); ++i) {
					out[i + 1] = texture.frames[i].path;
				}
				break;
			}
			return out;
		});
	}
}
