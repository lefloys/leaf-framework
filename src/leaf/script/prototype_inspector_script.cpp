#include "prototype_inspector_script.hpp"

#include <leaf/script/database.hpp>
#include <leaf/script/prototypes/texture.hpp>

namespace lf {
	void InstallPrototypeInspectorScript(sol::state& lua) {
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
