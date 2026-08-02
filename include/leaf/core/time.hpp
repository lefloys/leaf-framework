#pragma once
#include "leaf/core/array.hpp"
#include "leaf/core/format.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/unit.hpp"

namespace lf {
	using duration = unit<struct duration_tag>;
	using instant = unit<struct instant_tag>;
	using timespan = duration;

	instant now();

	constexpr instant operator+(instant lhs, duration rhs) {
		return instant::from_quantum(lhs.quantum_count() + rhs.quantum_count());
	}

	constexpr instant operator-(instant lhs, duration rhs) {
		return instant::from_quantum(lhs.quantum_count() - rhs.quantum_count());
	}

	constexpr duration operator-(instant lhs, instant rhs) {
		return duration::from_quantum(lhs.quantum_count() - rhs.quantum_count());
	}

	template<>
	struct unit_trait<timespan> {
		static constexpr array<std::pair<i64, string_view>, 4> units = {
			{ { 86'400'000'000'000LL, "d" },
			  { 3'600'000'000'000LL, "h" },
			  { 60'000'000'000LL, "m" },
			  { 1'000'000'000LL, "s" } }
		};
	};

	template<>
	struct pretty_string_trait<timespan> {
		static string to_string(const timespan& value);
		static timespan from_string(string_view str);
	};
} // namespace lf
