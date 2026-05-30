#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/filesystem.hpp>
#include <leaf/core/string.hpp>
#include <leaf/core/types.hpp>

namespace lf {
	struct SoundSettings {
		f32 master = 1.0f;
		f32 music = 0.8f;
		f32 effects = 1.0f;
	};

	struct GraphicsSettings {
		bool fullscreen = false;
		bool vsync = true;
		f32 max_fps = 60.0f;
	};

	struct AppSettings {
		string language = "en-US";
		string last_saved_world;
		SoundSettings sound;
		GraphicsSettings graphics;
	};

	AppSettings DefaultAppSettings();

	fs::path AppSettingsPath();
	report<AppSettings> LoadAppSettings(const fs::path& path);
	error SaveAppSettings(const fs::path& path, const AppSettings& settings);
	report<string> LoadSelectedLanguageSetting(const fs::path& path);
	error SaveSelectedLanguageSetting(const fs::path& path, string_view language);
	report<string> LoadLastSavedWorldSetting(const fs::path& path);
	error SaveLastSavedWorldSetting(const fs::path& path, string_view save_name);
}
