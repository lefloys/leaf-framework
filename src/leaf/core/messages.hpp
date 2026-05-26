#pragma once

#include "string.hpp"

namespace lf {
	string make_missing_field_error_message(string_view field_name);
	string make_type_mismatch_error_message(string_view expected, string_view actual);
	void log_info(string_view message);
	void log_warning(string_view message);
	void log_error(string_view message);
} // namespace lf
