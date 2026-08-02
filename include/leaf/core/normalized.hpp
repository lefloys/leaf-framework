#pragma once

#include "leaf/core/types.hpp"

#include <concepts>
#include <limits>
#include <type_traits>

namespace lf {
	template<typename T>
	concept normalized_integer = std::integral<T> && !std::same_as<T, bool>;

	template<normalized_integer T>
	struct normalized {
		static constexpr T minimum = std::numeric_limits<T>::lowest();
		static constexpr T maximum = std::numeric_limits<T>::max();

		constexpr normalized() = default;
		constexpr explicit normalized(T value) : value(value) {}

		constexpr T raw() const {
			return value;
		}

		template<std::floating_point F>
		explicit constexpr operator F() const {
			if constexpr (std::is_signed_v<T>) {
				if (value < 0) {
					return static_cast<F>(value) / -static_cast<F>(minimum);
				}
			}
			return static_cast<F>(value) / static_cast<F>(maximum);
		}

		constexpr bool operator==(const normalized&) const = default;

	  private:
		T value = 0;
	};
}
