#include "leaf/resource/prototypes/sound.hpp"

#include "leaf/audio/sound.hpp"
#include "leaf/core/format.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/resource/database.hpp"
#include "leaf/script/virtual_filesystem.hpp"
#include "leaf/script/extensions.hpp"

#include <sol/sol.hpp>

namespace lf {
	void InstallSoundScript(sol::state& lua) {
		lua.set_function("play_sound", [](SoundPrototype::ID::vnum_t sound_value, SoundGroupPrototype::ID::vnum_t group_value, f32 volume) {
			const SoundPrototype& sound = Database<SoundPrototype>::get(SoundPrototype::ID{ sound_value });
			const SoundGroupPrototype& group = Database<SoundGroupPrototype>::get(SoundGroupPrototype::ID{ group_value });
			return !PlaySound(sound.sound, volume * sound.volume * group.volume);
		});
	}

	SoundPrototype::SoundPrototype(const dict& data) : Prototype{ data } {
		data.assign(schema(*this));
	}


	error SoundPrototype::load() {
		auto resolved = ResolveVirtualPathReport(path);
		if (!resolved) {
			log::Warning("{}", lf::format("[sound] {}", resolved.error().message));
			return {};
		}
		auto bytes = fs::Read(resolved->string(), lf::tags::Binary);
		if (!bytes) {
			log::Warning("{}", lf::format("[sound] {}", bytes.error().message));
			return {};
		}
		auto loaded = LoadSound(lf::span<const lf::byte>(bytes->data(), bytes->size()));
		if (!loaded) {
			log::Warning("{}", lf::format("[sound] {}", loaded.error().message));
			return {};
		}
		sound = std::move(*loaded);
		return {};
	}

} // namespace lf

