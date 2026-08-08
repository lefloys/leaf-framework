#include "leaf/system/system.hpp"
#include "leaf/system/socket.hpp"
#include <Shlobj.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <windows.h>

namespace lf {
	struct SystemData {
		char appdata_dir[MAX_PATH] = { 0 };
		char install_dir[MAX_PATH] = { 0 };
	};

	struct ShutdownHandlerEntry {
		ShutdownHandlerId id = 0;
		ShutdownHandler handler = nullptr;
		void* user_data = nullptr;
	};

	static SystemData system_data;
	static std::vector<ShutdownHandlerEntry> shutdown_handlers;
	static ShutdownHandlerId next_shutdown_handler_id = 1;

	void RequestShutdown() {
		for (const ShutdownHandlerEntry& entry : shutdown_handlers) {
			if (entry.handler) {
				entry.handler(entry.user_data);
			}
		}
	}

	void ShowErrorBox(string_view title, string_view message) {
		if (const char* suppress_dialogs = std::getenv("LEAF_NO_ERROR_DIALOGS"); suppress_dialogs && suppress_dialogs[0]) {
			std::fprintf(stderr, "%.*s: %.*s\n", static_cast<int>(title.size()), title.data(), static_cast<int>(message.size()), message.data());
			std::fflush(stderr);
			return;
		}
		const string title_string(title);
		const string message_string(message);
		MessageBoxA(nullptr, message_string.c_str(), title_string.c_str(), MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
	}

	static BOOL WINAPI console_shutdown_handler(DWORD control_type) {
		switch (control_type) {
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			RequestShutdown();
			return TRUE;
		default:
			return FALSE;
		}
	}

	error init_system(span<string_view> args) {
		install_crash_handler();
		if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, system_data.appdata_dir))) {
			system_data.appdata_dir[MAX_PATH - 1] = '\0';
		} else {
			system_data.appdata_dir[0] = '\0';
		}
		if (const char* appdata_override = std::getenv("LEAF_APPDATA_DIR"); appdata_override && appdata_override[0]) {
			OverwriteAppdataDir(appdata_override);
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
		return sys::init_udp_sockets();
	}
	void exit_system() {
		sys::exit_udp_sockets();
	}

	string_view system_backend_name() {
		return "Windows";
	}

	ShutdownHandlerId AddShutdownHandler(ShutdownHandler handler, void* user_data) {
		const ShutdownHandlerId id = next_shutdown_handler_id++;
		shutdown_handlers.push_back(ShutdownHandlerEntry{
			.id = id,
			.handler = handler,
			.user_data = user_data,
		});
		SetConsoleCtrlHandler(console_shutdown_handler, TRUE);
		return id;
	}

	void RemoveShutdownHandler(ShutdownHandlerId id) {
		for (size_t index = 0; index < shutdown_handlers.size(); ++index) {
			if (shutdown_handlers[index].id != id) {
				continue;
			}
			shutdown_handlers.erase(shutdown_handlers.begin() + static_cast<std::ptrdiff_t>(index));
			break;
		}
		if (shutdown_handlers.empty()) {
			SetConsoleCtrlHandler(console_shutdown_handler, FALSE);
		}
	}

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
} // namespace lf
