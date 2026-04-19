#pragma once

#include "types.hpp"
#include "expected.hpp"
#include "format.hpp"
#include "string.hpp"

#include <stdexcept>
#include <system_error>

#define IF_ERROR_RETURN_ERROR(expr) do { if (auto err = (expr); err) { return err; } } while(0)

namespace lf {
	enum class generic_errc : i32;
}
template<> struct std::is_error_code_enum<lf::generic_errc> : std::true_type {};

namespace lf {
	using std::error_code;
	using std::error_category;
	using std::errc;

	enum class generic_errc : i32 {
		unknown = 1,
		parse_error,
		invalid_id,
		missing_field,
		input_error,
		type_mismatch,
	};


	struct generic_error_category : public std::error_category {
		const char* name() const noexcept override;
		string message(i32 ev) const override;
	};

	const error_category& generic_category();
	error_code make_error_code(generic_errc e);

	struct error {
		static const error no_error;
		static const error unknown_error;
		error() = default;
		template <typename error_enum> requires std::is_error_code_enum_v<error_enum>
		error(error_enum e, string_view msg = "");
		error(error_code c, string_view msg = "") : code(c), message(string(msg)) {}
		error(string_view msg) : code(make_error_code(generic_errc::unknown)), message(string(msg)) {}
		explicit operator bool() const noexcept;

		error& add_context(string_view context);

		string message;
		error_code code;
	};

	template <typename error_enum> requires std::is_error_code_enum_v<error_enum>
	error::error(error_enum e, string_view msg) : code(make_error_code(e)), message(string(msg)) {}

	template<typename T> using result = expected<T, error_code>;
	template<typename T> using report = expected<T, error>;
}
