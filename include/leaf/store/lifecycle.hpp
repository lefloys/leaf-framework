#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/span.hpp>
#include <leaf/core/string.hpp>

namespace lf {
	error init_store(span<string_view> args);
	string_view store_backend_name();
	void update_store();
	void exit_store();
}
