#include "leaf/resource/prototypes/sound.hpp"

#include "leaf/audio/sound.hpp"
#include "leaf/core/format.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/resource/database.hpp"
#include "leaf/script/virtual_filesystem.hpp"
#include "leaf/script/settings.hpp"
#include "leaf/script/extensions.hpp"

#include <sol/sol.hpp>

namespace lf {
	static void register_sound_extensions(sol::state& lua) {
		lua.set_function("play_sound", [](string_view name, sol::object group_object, f32 volume) {
			const SoundPrototype::ID id = Database<SoundPrototype>::find(name);
			if (!id) {
				return false;
			}
			const SoundPrototype& sound = Database<SoundPrototype>::get(id);
			SoundGroupPrototype::ID group = sound.group;
			if (group_object.is<sol::table>()) {
				const sol::object group_id = group_object.as<sol::table>()["id"];
				if (group_id.is<string>()) {
					group = Database<SoundGroupPrototype>::find(group_id.as<string>());
				}
			}
			if (!group) {
				group = Database<SoundGroupPrototype>::find("effects");
			}
			const f32 master = LoadSetting("core", "sound.master", 1.0)->as<f32>();
			const f32 group_volume = Database<SoundGroupPrototype>::get(group).volume;
			return !PlaySound(sound.sound, volume * sound.volume * group_volume * master);
		});
	}

	namespace {
		struct sound_extension_registration {
			sound_extension_registration() {
				script_system::register_installer(register_sound_extensions);
			}
		};

		const sound_extension_registration registered_sound_extensions{};
	}

	SoundPrototype::SoundPrototype(const dict& data) : Prototype(data) {
		load_field(data, "path", path);
		if (has_field(data, "sound_group")) {
			string group;
			load_field(data, "sound_group", group);
			this->group = Database<SoundGroupPrototype>::find(group);
		}
		if (has_field(data, "volume")) {
			load_field(data, "volume", volume);
		}
	}


	error SoundPrototype::load() {
		if (sound.channels != 0 || !sound.samples.empty() || path.empty()) {
			return {};
		}
		auto resolved = ResolveVirtualPathReport(path);
		if (!resolved) {
			log::Warning("{}", lf::format("[sound] {}", resolved.error().message));
			return {};
		}
		auto bytes = fs::ReadBinaryFile(resolved->string());
		if (!bytes) {
			log::Warning("{}", lf::format("[sound] {}", bytes.error().message));
			return {};
		}
		auto loaded = LoadSound(*bytes);
		if (!loaded) {
			log::Warning("{}", lf::format("[sound] {}", loaded.error().message));
			return {};
		}
		sound = std::move(*loaded);
		return {};
	}
} // namespace lf

