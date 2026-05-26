#include "leaf/system/system.hpp"
#include "leaf/core/format.hpp"

#include <Shlobj.h>
#include <cstring>
#include <fstream>
#include <iterator>
#include <windows.h>

namespace lf {
	struct SystemData {
		char appdata_dir[MAX_PATH] = { 0 };
		char install_dir[MAX_PATH] = { 0 };
	};

	static SystemData system_data;

	error init_system(span<string_view> args) {
		install_crash_handler();
		if (SUCCEEDED(
				SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, system_data.appdata_dir))) {
			system_data.appdata_dir[MAX_PATH - 1] = '\0';
		} else {
			system_data.appdata_dir[0] = '\0';
		}

		DWORD length = GetModuleFileNameA(nullptr, system_data.install_dir, MAX_PATH);
		if (length == 0 || length >= MAX_PATH) {
			system_data.install_dir[0] = '\0';
			return error::unknown_error;
		}
		for (DWORD i = length; i > 0; --i) {
			if (system_data.install_dir[i - 1] == '\\' || system_data.install_dir[i - 1] == '/') {
				system_data.install_dir[i - 1] = '\0';
				break;
			}
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
		size_t len = (new_path.size() < MAX_PATH - 1) ? new_path.size() : (MAX_PATH - 1);
		memcpy(system_data.appdata_dir, new_path.data(), len);
		system_data.appdata_dir[len] = '\0';
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
