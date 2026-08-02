#pragma once

#include "leaf/core/exception.hpp"
#include "leaf/core/format.hpp"
#include "leaf/core/typename.hpp"

#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

namespace lf {

	namespace detail {
		template<typename To, typename From>
		[[noreturn]] void throw_safe_cast_failure(const From& value) {
			throw safe_cast_exception(lf::format(
				"safe_cast from '{}' value {} to '{}' would lose data",
				lf::type_name<From>(),
				value,
				lf::type_name<To>()
			));
		}

		template<typename To, typename From>
		bool floating_to_integral_in_range(From value) {
			if (!std::isfinite(value) || std::trunc(value) != value) {
				return false;
			}

			const long double numeric_value = static_cast<long double>(value);
			if constexpr (std::is_signed_v<To>) {
				const long double lower = -std::ldexp(1.0L, std::numeric_limits<To>::digits);
				const long double upper = std::ldexp(1.0L, std::numeric_limits<To>::digits);
				return numeric_value >= lower && numeric_value < upper;
			} else {
				const long double upper = std::ldexp(1.0L, std::numeric_limits<To>::digits);
				return numeric_value >= 0.0L && numeric_value < upper;
			}
		}
	} // namespace detail

	template<typename To, typename From>
		requires(std::is_arithmetic_v<To> && std::is_arithmetic_v<From> && !std::is_same_v<std::remove_cv_t<To>, bool> && !std::is_same_v<std::remove_cv_t<From>, bool>)
	To safe_cast(From value) {
		using to_t = std::remove_cv_t<To>;
		using from_t = std::remove_cv_t<From>;

		if constexpr (std::is_same_v<to_t, from_t>) {
			return value;
		} else if constexpr (std::is_integral_v<to_t> && std::is_integral_v<from_t>) {
			if (!std::in_range<to_t>(value)) {
				detail::throw_safe_cast_failure<To, From>(value);
			}
		} else if constexpr (std::is_integral_v<to_t> && std::is_floating_point_v<from_t>) {
			if (!detail::floating_to_integral_in_range<to_t>(value)) {
				detail::throw_safe_cast_failure<To, From>(value);
			}
		} else if constexpr (std::is_floating_point_v<to_t>) {
			const to_t converted = static_cast<to_t>(value);
			if (std::isfinite(value) && !std::isfinite(converted)) {
				detail::throw_safe_cast_failure<To, From>(value);
			}
			if constexpr (std::is_integral_v<from_t>) {
				if (!detail::floating_to_integral_in_range<from_t>(converted) ||
					static_cast<from_t>(converted) != value) {
					detail::throw_safe_cast_failure<To, From>(value);
				}
			} else if (std::isfinite(value) && static_cast<from_t>(converted) != value) {
				detail::throw_safe_cast_failure<To, From>(value);
			}
		}

		return static_cast<To>(value);
	}
} // namespace lf
