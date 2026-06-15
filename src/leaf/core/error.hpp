#pragma once

#include "exception.hpp"
#include "format.hpp"
#include "string.hpp"
#include "vector.hpp"

#include <expected>
#include <stdexcept>
#include <system_error>

#define IF_ERROR_RETURN_ERROR(expr)   \
	do {                              \
		if (auto err = (expr); err) { \
			return err;               \
		}                             \
	} while (0)

namespace lf {
	enum class generic_errc : i32;
	enum class graphics_errc : i32;
} // namespace lf
template <>
struct std::is_error_code_enum<lf::generic_errc> : std::true_type {};
template <>
struct std::is_error_code_enum<lf::graphics_errc> : std::true_type {};

namespace lf {
	using std::errc;
	using std::error_category;
	using std::error_code;

	/*!
	** @brief Leaf-specific generic error codes.
	*/
	enum class generic_errc : i32 {
		unknown = 1,
		input_error,
		invalid_id,
		missing_field,
		parse_error,
		type_mismatch,
	};
	
	enum class graphics_errc : i32 {
		unknown = 1,
		already_exists,
		device_lost,
		extension_not_present,
		incompatible_driver,
		initialization,
		invalid_argument,
		layer_not_present,
		not_supported,
		out_of_device_memory,
		out_of_host_memory,
		platform_failure,
		shader_compilation,
		shader_link_failed,
		unsupported_feature,
	};

	/*!
	** @brief Error category backing generic_errc values.
	*/
	struct generic_error_category : public std::error_category {
		/*!
		** @brief Gets the stable category name.
		*/
		const ch08* name() const noexcept override;

		/*!
		** @brief Converts a generic_errc value to a diagnostic message.
		*/
		string message(i32 ev) const override;
	};

	struct graphics_error_category : public std::error_category {
		const ch08* name() const noexcept override;
		string message(i32 ev) const override;
	};

	/*!
	** @brief Gets Leaf's generic error category singleton.
	*/
	const error_category& generic_category();

	/*!
	** @brief Creates an error_code for a Leaf generic error.
	*/
	error_code make_error_code(generic_errc e);
	error_code make_error_code(graphics_errc e);

	/*!
	** @brief Error value with both a machine-readable code and human text.
	*/
	struct error {
		/*!
		** @brief Empty success value.
		*/
		static const error no_error;

		/*!
		** @brief Generic fallback failure value.
		*/
		static const error unknown_error;

		/*!
		** @brief Creates an empty success value.
		*/
		error() = default;

		/*!
		** @brief Creates an error from an error-code enum.
		*/
		template <typename error_enum>
			requires std::is_error_code_enum_v<error_enum>
		error(error_enum e, string_view msg = "");

		/*!
		** @brief Creates an error from a standard error_code.
		*/
		error(error_code c, string_view msg = "") : code(c), message(string(msg)) {}

		/*!
		** @brief Creates an unknown error with a text message.
		*/
		error(string_view msg)
			: code(make_error_code(generic_errc::unknown)), message(string(msg)) {}

		/*!
		** @brief Checks whether this value represents failure.
		*/
		explicit operator bool() const noexcept;

		/*!
		** @brief Adds context text to the diagnostic message.
		*/
		error& add_context(string_view context);

		/*!
		** @brief Human-readable diagnostic message.
		*/
		string message;

		/*!
		** @brief Machine-readable error code.
		*/
		error_code code;
	};

	template <typename error_enum>
		requires std::is_error_code_enum_v<error_enum>
	error::error(error_enum e, string_view msg) : code(make_error_code(e)), message(string(msg)) {}

	/*!
	** @brief Expected result that reports only an error_code on failure.
	*/
	template <typename T>
	using result = std::expected<T, error_code>;

	/*!
	** @brief Expected result that reports a rich Leaf error on failure.
	*/
	template <typename T>
	using report = std::expected<T, error>;
	using std::unexpected;
} // namespace lf
