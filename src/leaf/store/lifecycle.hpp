#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/span.hpp>
#include <leaf/core/string.hpp>

namespace lf {
	error init_store(span<string_view> args);
	void update_store();
	void exit_store();
}
