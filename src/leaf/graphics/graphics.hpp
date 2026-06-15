#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"

#include <rutile.h>

namespace lf {
	string_view GraphicsBackendName();

	error init_graphics(span<string_view> args, bool headless);
	void exit_graphics();
	error rutile_error();
}
