#include "error.hpp"

namespace lf {
	const error error::no_error = error();
	const error error::unknown_error = error(generic_errc::unknown);
	const char* generic_error_category::name() const noexcept {
		return "generic";
	}

	string generic_error_category::message(i32 ev) const {
		switch (static_cast<generic_errc>(ev)) {
		case generic_errc::unknown: return "Unknown error";
		case generic_errc::parse_error: return "Parse error";
		case generic_errc::invalid_id: return "Invalid id";
		case generic_errc::missing_field: return "Missing field";
		case generic_errc::input_error: return "Input error";
		case generic_errc::type_mismatch: return "Type mismatch";
		default: return "Unrecognized error code";
		}
	}

	const error_category& generic_category() {
		static generic_error_category instance;
		return instance;
	}

	error_code make_error_code(generic_errc e) {
		return { static_cast<i32>(e), generic_category() };
	}

	error::operator bool() const noexcept {
		return code.value() != 0;
	}
	error& error::add_context(string_view context) {
		if (context.empty()) {
			return *this;
		}
		message = format("{}\n -> {}", context, message);
		return *this;
	}

} // namespace lf
