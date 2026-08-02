#include "leaf/audio/sound.hpp"

#include "leaf/core/format.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/resource/database.hpp"
#include "leaf/resource/prototypes/sound.hpp"
#include "leaf/script/settings.hpp"
#include "leaf/script/virtual_filesystem.hpp"

#include <memory>
#include <mutex>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef PlaySound
#undef PlaySound
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
} // namespace lf

struct ActiveSound {
	std::reference_wrapper<const lf::Sound> asset;
	ma_audio_buffer buffer{};
	ma_sound sound{};
	bool buffer_initialized = false;
	bool sound_initialized = false;

	explicit ActiveSound(const lf::Sound& asset) : asset(asset) {}

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

	lf::error play(const lf::Sound& asset, f32 volume) {
		if (!initialized) {
			return lf::error(lf::generic_errc::input_error, "failed to initialize audio engine");
		}
		if (asset.channels == 0 || asset.samples.empty()) {
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

		auto active = std::make_unique<ActiveSound>(asset);
		const ma_uint64 frame_count = static_cast<ma_uint64>(asset.samples.size() / asset.channels);
		ma_audio_buffer_config buffer_config = ma_audio_buffer_config_init(
			ma_format_f32, asset.channels, frame_count,
			const_cast<float*>(asset.samples.data()), nullptr
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

namespace lf {
	report<Sound> LoadSound(span<const byte> bytes) {
		ma_decoder decoder{};
		ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
		if (ma_decoder_init_memory(bytes.data(), bytes.size(), &config, &decoder) != MA_SUCCESS) {
			return unexpected(error(generic_errc::input_error, "failed to decode sound bytes"));
		}

		ma_uint64 frame_count = 0;
		if (ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count) != MA_SUCCESS ||
			frame_count == 0 || decoder.outputChannels == 0) {
			ma_decoder_uninit(&decoder);
			return unexpected(error(generic_errc::input_error, "sound bytes are empty"));
		}

		Sound asset;
		asset.channels = decoder.outputChannels;
		asset.sample_rate = decoder.outputSampleRate;
		asset.samples.resize(static_cast<size_t>(frame_count) * asset.channels);
		ma_uint64 frames_read = 0;
		const ma_result read_result = ma_decoder_read_pcm_frames(
			&decoder, asset.samples.data(), frame_count, &frames_read
		);
		ma_decoder_uninit(&decoder);
		if (read_result != MA_SUCCESS || frames_read == 0) {
			return unexpected(error(generic_errc::input_error, "failed to read sound bytes"));
		}
		asset.samples.resize(static_cast<size_t>(frames_read) * asset.channels);
		return asset;
	}

	error PlaySound(const Sound& sound, f32 volume) {
		if (volume <= 0.0f) {
			return error::no_error;
		}
		return sound_engine().play(sound, volume);
	}

} // namespace lf

