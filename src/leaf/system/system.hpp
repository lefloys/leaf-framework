#pragma once
#include "leaf/core/string.hpp"

namespace lf {
	void InitSystem();
	void OverwriteAppdataDir(string_view new_path);
	string_view GetAppdataDir();
	string_view GetInstallDir();
} // namespace lf
