#include "leaf/script/prototype_inspector_script.hpp"

#include <leaf/resource/database.hpp>
#include <leaf/resource/prototypes/texture.hpp>

namespace lf {
	void InstallPrototypeInspectorScript(sol::state& lua) {
		lua.set_function("texture_animation_fps", [](string_view texture_name) {
			for (size_t index = 0; index < Database<TexturePrototype>::prototypes.size(); ++index) {
				const auto id = TexturePrototype::ID{ static_cast<TexturePrototype::ID::vnum_t>(index + 1) };
				const TexturePrototype& texture = Database<TexturePrototype>::get(id);
				if (Database<TexturePrototype>::name(id) == texture_name) {
					return texture.frames_per_second > 0.0f ? texture.frames_per_second : 1.0f;
				}
			}
			return 1.0f;
		});

		lua.set_function("texture_animation_frames", [&lua](string_view texture_name) {
			sol::table out = lua.create_table();
			for (size_t index = 0; index < Database<TexturePrototype>::prototypes.size(); ++index) {
				const auto id = TexturePrototype::ID{ static_cast<TexturePrototype::ID::vnum_t>(index + 1) };
				const TexturePrototype& texture = Database<TexturePrototype>::get(id);
				if (Database<TexturePrototype>::name(id) != texture_name) {
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
} // namespace lf
