#include "messages.hpp"

#include "format.hpp"

#include <iostream>

namespace lf {
	string make_missing_field_error_message(string_view field_name) {
		return format("missing required field '{}'", field_name);
	}

	string make_type_mismatch_error_message(string_view expected, string_view actual) {
		return format("expected type '{}', but got type '{}'", expected, actual);
	}

	void log_info(string_view message) {
		std::cout << message << '\n';
	}

	void log_warning(string_view message) {
		std::cout << "warning : " << message << '\n';
	}

	void log_error(string_view message) {
		std::cout << message << '\n';
	}
} // namespace lf
