#include "leaf/system/system.hpp"
#include "leaf/core/format.hpp"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits.h>
#include <unistd.h>

namespace lf {
	struct SystemData {
		string appdata_dir;
		string install_dir;
	};

	static SystemData system_data;

	error init_system(span<string_view> args) {
		install_crash_handler();
		const char* xdg_data_home = std::getenv("XDG_DATA_HOME");
		if (xdg_data_home && xdg_data_home[0]) {
			system_data.appdata_dir = xdg_data_home;
		} else if (const char* home = std::getenv("HOME"); home && home[0]) {
			system_data.appdata_dir = (std::filesystem::path(home) / ".local" / "share").string();
		} else {
			system_data.appdata_dir = ".";
		}

		char executable_path[PATH_MAX] = {};
		ssize_t length = readlink("/proc/self/exe", executable_path, sizeof(executable_path) - 1);
		if (length > 0) {
			executable_path[length] = '\0';
			system_data.install_dir = std::filesystem::path(executable_path).parent_path().string();
		} else if (!args.empty()) {
			system_data.install_dir = std::filesystem::absolute(std::filesystem::path(string(args[0]))).parent_path().string();
		} else {
			system_data.install_dir = std::filesystem::current_path().string();
		}
		return error::no_error;
	}

	void exit_system() {}

	string_view GetAppdataDir() {
		return system_data.appdata_dir;
	}

	string_view GetInstallDir() {
		return system_data.install_dir;
	}

	void OverwriteAppdataDir(string_view new_path) {
		system_data.appdata_dir = new_path;
	}

	report<string> ReadTextFile(string_view path) {
		std::ifstream file(string(path), std::ios::binary);
		if (!file) {
			return unexpected(error(
				generic_errc::input_error,
				format("failed to open '{}'", path)));
		}

		string text(
			(std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
		if (!file.eof() && file.fail()) {
			return unexpected(error(
				generic_errc::input_error,
				format("failed to read '{}'", path)));
		}
		return text;
	}
} // namespace lf
