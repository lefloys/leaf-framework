#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"

namespace lf {
	error init_graphics(span<string_view> args, bool headless = false);
	void LogGraphicsInfo();
	void exit_graphics();
	string_view graphics_backend_name();
}
