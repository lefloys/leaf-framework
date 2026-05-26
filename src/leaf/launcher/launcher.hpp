#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/filesystem.hpp>
#include <leaf/core/span.hpp>
#include <leaf/core/string.hpp>

namespace lf {
	error init_launcher(span<string_view> args);
	void update_launcher();
	void exit_launcher();
} // namespace lf
