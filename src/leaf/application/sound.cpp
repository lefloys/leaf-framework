#include "sound.hpp"

#include <leaf/core/format.hpp>
#include <leaf/logging/logging.hpp>
#include <leaf/script/database.hpp>
#include <leaf/script/settings.hpp>
#include <leaf/script/sound_prototype.hpp>
#include <leaf/script/virtual_filesystem.hpp>

#include <memory>
#include <mutex>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#ifdef _WIN32
static ma_result safe_ma_sound_init_from_file(ma_engine* engine, const char* path, ma_uint32 flags, ma_sound* sound) {
	__try {
		return ma_sound_init_from_file(engine, path, flags, nullptr, nullptr, sound);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return MA_ERROR;
	}
}

static ma_result safe_ma_sound_start(ma_sound* sound) {
	__try {
		return ma_sound_start(sound);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return MA_ERROR;
	}
}
#else
static ma_result safe_ma_sound_init_from_file(ma_engine* engine, const char* path, ma_uint32 flags, ma_sound* sound) {
	return ma_sound_init_from_file(engine, path, flags, nullptr, nullptr, sound);
}

static ma_result safe_ma_sound_start(ma_sound* sound) {
	return ma_sound_start(sound);
}
#endif

struct SoundEngineState {
	ma_engine engine{};
	bool initialized = false;
	std::mutex mutex;
	std::vector<std::unique_ptr<ma_sound>> sounds;

	SoundEngineState() {
		initialized = ma_engine_init(nullptr, &engine) == MA_SUCCESS;
	}

	~SoundEngineState() {
		for (auto& sound : sounds) {
			ma_sound_uninit(sound.get());
		}
		if (initialized) {
			ma_engine_uninit(&engine);
		}
	}

	lf::error play(const lf::fs::path& path, f32 volume) {
		if (!initialized) {
			return lf::error(lf::generic_errc::input_error, "failed to initialize audio engine");
		}

		std::lock_guard lock(mutex);
		for (auto it = sounds.begin(); it != sounds.end();) {
			if (ma_sound_at_end(it->get())) {
				ma_sound_uninit(it->get());
				it = sounds.erase(it);
			} else {
				++it;
			}
		}

		auto sound = std::make_unique<ma_sound>();
		ma_result result = safe_ma_sound_init_from_file(&engine, path.string().c_str(), 0, sound.get());
		if (result != MA_SUCCESS) {
			return lf::error(lf::generic_errc::input_error, lf::format("failed to load sound '{}'", path.string()));
		}
		ma_sound_set_volume(sound.get(), volume);
		result = safe_ma_sound_start(sound.get());
		if (result != MA_SUCCESS) {
			ma_sound_uninit(sound.get());
			return lf::error(lf::generic_errc::input_error, lf::format("failed to play sound '{}'", path.string()));
		}
		sounds.push_back(std::move(sound));
		return lf::error::no_error;
	}
};

static SoundEngineState& sound_engine() {
	static SoundEngineState engine;
	return engine;
}

static lf::string sound_type_id(const sol::object& value) {
	if (value == sol::nil) {
		return {};
	}
	if (value.is<lf::string>()) {
		return value.as<lf::string>();
	}
	if (value.is<sol::table>()) {
		sol::table table = value.as<sol::table>();
		sol::object id = table["id"];
		if (id.is<lf::string>()) {
			return id.as<lf::string>();
		}
	}
	return {};
}

static f32 clamp_volume(f32 value) {
	if (value < 0.0f) {
		return 0.0f;
	}
	if (value > 1.0f) {
		return 1.0f;
	}
	return value;
}

static f32 settings_volume_for_sound_type(lf::string_view type) {
	auto master_setting = lf::LoadSetting("core", "sound.master", 1.0);
	auto effects_setting = lf::LoadSetting("core", "sound.effects", 1.0);
	f32 master = master_setting ? master_setting->as<f32>() : 1.0f;
	f32 type_volume = effects_setting ? effects_setting->as<f32>() : 1.0f;
	if (type == "music") {
		if (auto music_setting = lf::LoadSetting("core", "sound.music", 0.8)) {
			type_volume = music_setting->as<f32>();
		}
	} else if (type == "master") {
		type_volume = 1.0f;
	}
	return clamp_volume(master) * clamp_volume(type_volume);
}

namespace lf {
	error PlaySoundFile(const fs::path& path, f32 volume) {
		if (volume <= 0.0f) {
			return error::no_error;
		}
		if (!fs::exists(path)) {
			return error(generic_errc::input_error, format("missing sound '{}'", path.string()));
		}

		return sound_engine().play(path, volume);
	}

	void InstallSoundScript(sol::state& lua) {
		lua.set_function("play_sound", [](string_view name, sol::object type, f32 volume) {
			identifier<SoundPrototype, u16, void> id = Database<SoundPrototype>::find(name);
			if (!id) {
				log::Warning("{}", format("[sound] missing sound prototype '{}'", name));
				return false;
			}

			const SoundPrototype& sound = Database<SoundPrototype>::get(id);
			string type_id = sound_type_id(type);
			if (type_id.empty()) {
				type_id = sound.sound_type;
			}

			f32 final_volume = clamp_volume(volume) * clamp_volume(sound.volume) * settings_volume_for_sound_type(type_id);
			auto path = ResolveVirtualPathReport(sound.path);
			if (!path) {
				log::Warning("{}", format("[sound] {}", path.error().message));
				return false;
			}

			if (error err = PlaySoundFile(*path, final_volume)) {
				log::Warning("{}", format("[sound] {}", err.message));
				return false;
			}
			return true;
		});
	}
}
