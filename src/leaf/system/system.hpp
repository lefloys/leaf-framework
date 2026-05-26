#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"


namespace lf {
	void install_crash_handler();
	error init_system(span<string_view> args);
	void exit_system();
	void OverwriteAppdataDir(string_view new_path);
	string_view GetAppdataDir();
	string_view GetInstallDir();
	report<string> ReadTextFile(string_view path);
} // namespace lf
