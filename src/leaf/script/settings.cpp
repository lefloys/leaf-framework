#include "settings.hpp"

#include <leaf/core/format.hpp>
#include <leaf/script/localization.hpp>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <sstream>

namespace lf {
	namespace {
		string trim(string_view value) {
			size_t begin = 0;
			size_t end = value.size();
			while (begin < end && std::isspace(static_cast<unsigned char>(value[begin]))) {
				++begin;
			}
			while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
				--end;
			}
			return string(value.substr(begin, end - begin));
		}

		string unquote_setting_value(string value) {
			if (value.size() < 2) {
				return value;
			}

			char quote = value.front();
			if ((quote != '"' && quote != '\'') || value.back() != quote) {
				return value;
			}
			return string(string_view(value).substr(1, value.size() - 2));
		}

		report<f32> parse_f32(string_view text, const fs::path& path, size_t line_number) {
			string value = trim(text);
			f32 result = 0.0f;
			const char* begin = value.data();
			const char* end = value.data() + value.size();
			auto [ptr, ec] = std::from_chars(begin, end, result);
			if (ec != std::errc() || ptr != end) {
				return unexpected(error(
					generic_errc::parse_error,
					format("{}:{} expected number", path.string(), line_number)));
			}
			return result;
		}

		report<bool> parse_bool(string_view text, const fs::path& path, size_t line_number) {
			string value = trim(text);
			std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			if (value == "true" || value == "1") {
				return true;
			}
			if (value == "false" || value == "0") {
				return false;
			}
			return unexpected(error(
				generic_errc::parse_error,
				format("{}:{} expected true or false", path.string(), line_number)));
		}

		string format_bool(bool value) {
			return value ? "true" : "false";
		}

		AppSettings normalize_settings(AppSettings settings) {
			if (settings.language.empty()) {
				settings.language = string(default_language);
			}
			settings.sound.master = std::clamp(settings.sound.master, 0.0f, 1.0f);
			settings.sound.music = std::clamp(settings.sound.music, 0.0f, 1.0f);
			settings.sound.effects = std::clamp(settings.sound.effects, 0.0f, 1.0f);
			return settings;
		}

		error write_settings_file(const fs::path& path, const AppSettings& settings) {
			AppSettings normalized = normalize_settings(settings);
			std::error_code ec;
			fs::create_directories(path.parent_path(), ec);
			if (ec) {
				return error(generic_errc::input_error, format("failed to create '{}': {}", path.parent_path().string(), ec.message()));
			}

			std::ofstream file(path, std::ios::binary);
			if (!file) {
				return error(generic_errc::input_error, format("failed to write '{}'", path.string()));
			}

			file << "language: " << normalized.language << "\n";
			file << "last_saved_world: \"" << normalized.last_saved_world << "\"\n";
			file << "sound:\n";
			file << "  master: " << normalized.sound.master << "\n";
			file << "  music: " << normalized.sound.music << "\n";
			file << "  effects: " << normalized.sound.effects << "\n";
			file << "graphics:\n";
			file << "  fullscreen: " << format_bool(normalized.graphics.fullscreen) << "\n";
			file << "  vsync: " << format_bool(normalized.graphics.vsync) << "\n";
			file << "  max_fps: " << normalized.graphics.max_fps << "\n";
			if (!file) {
				return error(generic_errc::input_error, format("failed to write '{}'", path.string()));
			}
			return error::no_error;
		}

		report<AppSettings> parse_settings(string_view text, const fs::path& path) {
			AppSettings settings = DefaultAppSettings();
			string section;
			string line;
			size_t line_number = 0;
			std::istringstream file{string(text)};
			while (std::getline(file, line)) {
				++line_number;
				if (line_number == 1 && line.starts_with("\xEF\xBB\xBF")) {
					line.erase(0, 3);
				}

				string setting = trim(line);
				if (setting.empty() || setting[0] == '#' || setting[0] == ';') {
					continue;
				}

				size_t colon = setting.find(':');
				if (colon == string::npos) {
					return unexpected(error(
						generic_errc::parse_error,
						format("{}:{} expected key: value", path.string(), line_number)));
				}

				string key = trim(string_view(setting).substr(0, colon));
				string value = trim(string_view(setting).substr(colon + 1));
				if (value.empty()) {
					section = key;
					continue;
				}

				if (section.empty()) {
					if (key == "language") {
						settings.language = unquote_setting_value(value);
					} else if (key == "last_saved_world") {
						settings.last_saved_world = unquote_setting_value(value);
					}
					continue;
				}

				if (section == "sound") {
					auto number = parse_f32(value, path, line_number);
					if (!number) {
						return unexpected(number.error());
					}
					if (key == "master") {
						settings.sound.master = *number;
					} else if (key == "music") {
						settings.sound.music = *number;
					} else if (key == "effects") {
						settings.sound.effects = *number;
					}
				} else if (section == "graphics") {
					if (key == "max_fps") {
						auto number = parse_f32(value, path, line_number);
						if (!number) {
							return unexpected(number.error());
						}
						settings.graphics.max_fps = *number;
						continue;
					}

					auto boolean = parse_bool(value, path, line_number);
					if (!boolean) {
						return unexpected(boolean.error());
					}
					if (key == "fullscreen") {
						settings.graphics.fullscreen = *boolean;
					} else if (key == "vsync") {
						settings.graphics.vsync = *boolean;
					}
				}
			}

			return normalize_settings(settings);
		}
	} // namespace

	AppSettings DefaultAppSettings() {
		AppSettings settings;
		settings.language = string(default_language);
		return settings;
	}

	fs::path AppSettingsPath() {
		return fs::folder::appdata / "settings.yaml";
	}

	report<AppSettings> LoadAppSettings(const fs::path& path) {
		if (!fs::exists(path)) {
			AppSettings settings = DefaultAppSettings();
			if (error err = write_settings_file(path, settings)) {
				return unexpected(err);
			}
			return settings;
		}

		auto text = fs::ReadTextFile(path.string());
		if (!text.has_value()) {
			error err = text.error();
			return unexpected(err.add_context(format("loading '{}'", path.string())));
		}

		auto settings = parse_settings(*text, path);
		if (!settings.has_value()) {
			error err = settings.error();
			return unexpected(err.add_context(format("loading '{}'", path.string())));
		}
		if (text->find("sound:") == string::npos || text->find("graphics:") == string::npos || text->find("max_fps:") == string::npos || text->find("last_saved_world:") == string::npos) {
			if (error err = write_settings_file(path, *settings)) {
				return unexpected(err.add_context(format("updating '{}'", path.string())));
			}
		}
		return *settings;
	}

	error SaveAppSettings(const fs::path& path, const AppSettings& settings) {
		return write_settings_file(path, settings);
	}

	report<string> LoadSelectedLanguageSetting(const fs::path& path) {
		auto settings = LoadAppSettings(path);
		if (!settings.has_value()) {
			return unexpected(settings.error());
		}
		return settings->language.empty() ? string(default_language) : settings->language;
	}

	error SaveSelectedLanguageSetting(const fs::path& path, string_view language) {
		auto settings = LoadAppSettings(path);
		if (!settings.has_value()) {
			return settings.error();
		}
		settings->language = language.empty() ? string(default_language) : string(language);
		return SaveAppSettings(path, *settings);
	}

	report<string> LoadLastSavedWorldSetting(const fs::path& path) {
		auto settings = LoadAppSettings(path);
		if (!settings.has_value()) {
			return unexpected(settings.error());
		}
		return settings->last_saved_world;
	}

	error SaveLastSavedWorldSetting(const fs::path& path, string_view save_name) {
		auto settings = LoadAppSettings(path);
		if (!settings.has_value()) {
			return settings.error();
		}
		settings->last_saved_world = string(save_name);
		return SaveAppSettings(path, *settings);
	}
} // namespace lf
