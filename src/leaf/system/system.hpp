#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"


namespace lf {
	using ShutdownHandler = void (*)(void*);
	using ShutdownHandlerId = u64;

	void install_crash_handler();
	error init_system(span<string_view> args);
	void exit_system();
	string_view system_backend_name();
	ShutdownHandlerId AddShutdownHandler(ShutdownHandler handler, void* user_data = nullptr);
	void RemoveShutdownHandler(ShutdownHandlerId id);
	void RequestShutdown();
	void OverwriteAppdataDir(string_view new_path);
	string_view GetAppdataDir();
	string_view GetInstallDir();
} // namespace lf
