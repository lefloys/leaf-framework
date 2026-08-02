#include "leaf/application/sound.hpp"

#include "leaf/core/format.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/script/database.hpp"
#include "leaf/script/prototypes/sound.hpp"
#include "leaf/script/settings.hpp"
#include "leaf/script/virtual_filesystem.hpp"

#include <memory>
#include <mutex>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#ifdef _WIN32
static ma_result safe_ma_sound_start(ma_sound* sound) {
	__try {
		return ma_sound_start(sound);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return MA_ERROR;
	}
}
#else
static ma_result safe_ma_sound_start(ma_sound* sound) {
	return ma_sound_start(sound);
}
#endif

namespace lf {
	struct SoundAsset {
		ma_uint32 channels = 0;
		ma_uint32 sample_rate = 0;
		std::vector<float> samples;
	};
} // namespace lf

struct ActiveSound {
	lf::SoundAssetHandle asset;
	ma_audio_buffer buffer{};
	ma_sound sound{};
	bool buffer_initialized = false;
	bool sound_initialized = false;

	~ActiveSound() {
		if (sound_initialized) {
			ma_sound_uninit(&sound);
		}
		if (buffer_initialized) {
			ma_audio_buffer_uninit(&buffer);
		}
	}
};

struct SoundEngineState {
	ma_engine engine{};
	bool initialized = false;
	std::mutex mutex;
	std::vector<std::unique_ptr<ActiveSound>> sounds;

	SoundEngineState() {
		initialized = ma_engine_init(nullptr, &engine) == MA_SUCCESS;
	}

	~SoundEngineState() {
		sounds.clear();
		if (initialized) {
			ma_engine_uninit(&engine);
		}
	}

	lf::error play(const lf::SoundAssetHandle& asset, f32 volume) {
		if (!initialized) {
			return lf::error(lf::generic_errc::input_error, "failed to initialize audio engine");
		}
		if (!asset || asset->channels == 0 || asset->samples.empty()) {
			return lf::error(lf::generic_errc::input_error, "sound asset is not loaded");
		}

		std::lock_guard lock(mutex);
		for (auto it = sounds.begin(); it != sounds.end();) {
			if (ma_sound_at_end(&(*it)->sound)) {
				it = sounds.erase(it);
			} else {
				++it;
			}
		}

		auto active = std::make_unique<ActiveSound>();
		active->asset = asset;
		const ma_uint64 frame_count = static_cast<ma_uint64>(asset->samples.size() / asset->channels);
		ma_audio_buffer_config buffer_config = ma_audio_buffer_config_init(
			ma_format_f32, asset->channels, frame_count,
			const_cast<float*>(asset->samples.data()), nullptr
		);
		ma_result result = ma_audio_buffer_init(&buffer_config, &active->buffer);
		if (result != MA_SUCCESS) {
			return lf::error(lf::generic_errc::input_error, "failed to create sound buffer");
		}
		active->buffer_initialized = true;
		result = ma_sound_init_from_data_source(&engine, &active->buffer, 0, nullptr, &active->sound);
		if (result != MA_SUCCESS) {
			return lf::error(lf::generic_errc::input_error, "failed to create sound voice");
		}
		active->sound_initialized = true;
		ma_sound_set_volume(&active->sound, volume);
		result = safe_ma_sound_start(&active->sound);
		if (result != MA_SUCCESS) {
			return lf::error(lf::generic_errc::input_error, "failed to play sound");
		}
		sounds.push_back(std::move(active));
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
	report<SoundAssetHandle> LoadSoundAsset(const fs::path& path) {
		if (!fs::exists(path)) {
			return unexpected(error(generic_errc::input_error, lf::format("missing sound '{}'", path.string())));
		}

		ma_decoder decoder{};
		ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
		if (ma_decoder_init_file(path.string().c_str(), &config, &decoder) != MA_SUCCESS) {
			return unexpected(error(generic_errc::input_error, lf::format("failed to decode sound '{}'", path.string())));
		}

		ma_uint64 frame_count = 0;
		if (ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count) != MA_SUCCESS ||
			frame_count == 0 || decoder.outputChannels == 0) {
			ma_decoder_uninit(&decoder);
			return unexpected(error(generic_errc::input_error, lf::format("sound '{}' is empty", path.string())));
		}

		auto asset = std::make_shared<SoundAsset>();
		asset->channels = decoder.outputChannels;
		asset->sample_rate = decoder.outputSampleRate;
		asset->samples.resize(static_cast<size_t>(frame_count) * asset->channels);
		ma_uint64 frames_read = 0;
		const ma_result read_result = ma_decoder_read_pcm_frames(
			&decoder, asset->samples.data(), frame_count, &frames_read
		);
		ma_decoder_uninit(&decoder);
		if (read_result != MA_SUCCESS || frames_read == 0) {
			return unexpected(error(generic_errc::input_error, lf::format("failed to read sound '{}'", path.string())));
		}
		asset->samples.resize(static_cast<size_t>(frames_read) * asset->channels);
		return SoundAssetHandle(std::move(asset));
	}

	error PlaySoundAsset(const SoundAssetHandle& asset, f32 volume) {
		if (volume <= 0.0f) {
			return error::no_error;
		}
		return sound_engine().play(asset, volume);
	}

	error PlaySoundFile(const fs::path& path, f32 volume) {
		if (volume <= 0.0f) {
			return error::no_error;
		}
		if (!fs::exists(path)) {
			return error(generic_errc::input_error, lf::format("missing sound '{}'", path.string()));
		}

		auto asset = LoadSoundAsset(path);
		return asset ? PlaySoundAsset(*asset, volume) : asset.error();
	}

	void InstallSoundScript(sol::state& lua) {
		lua.set_function("play_sound", [](string_view name, sol::object type, f32 volume) {
			SoundPrototype::ID id = Database<SoundPrototype>::find(name);
			if (!id) {
				log::Warning("{}", lf::format("[sound] missing sound prototype '{}'", name));
				return false;
			}

			const SoundPrototype& sound = Database<SoundPrototype>::get(id);
			string type_id = sound_type_id(type);
			if (type_id.empty()) {
				type_id = sound.sound_type;
			}

			f32 final_volume = clamp_volume(volume) * clamp_volume(sound.volume) * settings_volume_for_sound_type(type_id);
			if (error err = PlaySoundAsset(sound.asset, final_volume)) {
				log::Warning("{}", lf::format("[sound] {}", err.message));
				return false;
			}
			return true;
		});
	}
} // namespace lf
