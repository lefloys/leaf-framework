
#pragma once

#include "leaf/core/types.hpp"
#include "leaf/core/error.hpp"
#include "leaf/export.hpp"

#include <string_view>

namespace lf {
	struct WindowBackend;

	struct HostAPI {
		const WindowBackend* window_backend = nullptr;
	};

	LF_API error Init(i32 argc, char* argv[]);
	LF_API void Exit();

	LF_API error LoadWindowBackend(std::string_view module_path);
	LF_API error LoadGraphicsBackend(std::string_view module_path);
	LF_API const HostAPI* GetHostAPI();
}