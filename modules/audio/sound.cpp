#include "leaf/audio/sound.hpp"

#include "leaf/core/exception.hpp"
#include "leaf/core/format.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/core/singleton.hpp"
#include "leaf/resource/database.hpp"
#include "leaf/resource/prototypes/sound.hpp"
#include "leaf/script/virtual_filesystem.hpp"

#include <memory>
#include <mutex>
#include <vector>

#ifdef PlaySound
#undef PlaySound
#endif

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace lf {
	namespace {
		struct audio_buffer {
			ma_audio_buffer value{};

			explicit audio_buffer(const Sound& sound) {
				const ma_uint64 frame_count = static_cast<ma_uint64>(sound.samples.size() / sound.channels);
				const ma_audio_buffer_config config = ma_audio_buffer_config_init(
					ma_format_f32, sound.channels, frame_count, const_cast<f32*>(sound.samples.data()), nullptr
				);
				if (ma_audio_buffer_init(&config, &value) != MA_SUCCESS) {
					throw runtime_exception("failed to create sound buffer");
				}
			}

			~audio_buffer() {
				ma_audio_buffer_uninit(&value);
			}
		};

		struct sound_voice {
			ma_sound value{};

			sound_voice(ma_engine& engine, ma_audio_buffer& source) {
				if (ma_sound_init_from_data_source(&engine, &source, 0, nullptr, &value) != MA_SUCCESS) {
					throw runtime_exception("failed to create sound voice");
				}
			}

			~sound_voice() {
				ma_sound_uninit(&value);
			}
		};

		struct ActiveSound {
			Sound asset;
			audio_buffer buffer;
			sound_voice sound;

			ActiveSound(ma_engine& engine, const Sound& asset) : asset{ asset }, buffer{ this->asset }, sound{ engine, buffer.value } {}
		};

		struct SoundEngineState : Singleton<SoundEngineState> {
			ma_engine engine{};
			std::mutex mutex;
			std::vector<std::unique_ptr<ActiveSound>> sounds;

			SoundEngineState() {
				if (ma_engine_init(nullptr, &engine) != MA_SUCCESS) {
					throw runtime_exception("failed to initialize audio engine");
				}
			}

			~SoundEngineState() {
				sounds.clear();
				ma_engine_uninit(&engine);
			}

			error play(const Sound& asset, f32 volume) {
		if (asset.channels == 0 || asset.samples.empty()) {
			return error(generic_errc::input_error, "sound asset is not loaded");
		}

		std::lock_guard lock(mutex);
		for (auto it = sounds.begin(); it != sounds.end();) {
			if (ma_sound_at_end(&(*it)->sound.value)) {
				it = sounds.erase(it);
			} else {
				++it;
			}
		}

		auto active = std::make_unique<ActiveSound>(engine, asset);
		ma_sound_set_volume(&active->sound.value, volume);
		if (ma_sound_start(&active->sound.value) != MA_SUCCESS) {
			return error(generic_errc::input_error, "failed to play sound");
		}
		sounds.push_back(std::move(active));
		return error::no_error;
	}
		};
	} // namespace
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
		return SoundEngineState::instance().play(sound, volume);
	}

} // namespace lf

