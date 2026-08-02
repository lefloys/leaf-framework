#include "leaf/core/time.hpp"

#include <chrono>

namespace lf {
	instant now() {
		const std::chrono::steady_clock::duration duration = std::chrono::steady_clock::now().time_since_epoch();
		const i64 nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
		return instant::from_quantum(nanoseconds);
	}

	string pretty_string_trait<timespan>::to_string(const timespan& value) {
		i64 rem = value.value;
		string result;
		for (auto& u : unit_trait<timespan>::units) {
			i64 n = rem / u.first;
			if (n > 0) {
				if (!result.empty()) {
					result += " ";
				}
				result += lf::format("{}{}", n, u.second);
				rem %= u.first;
			}
		}
		if (result.empty()) {
			result = "0s";
		}
		return result;
	}
	timespan pretty_string_trait<timespan>::from_string(string_view str) {
		string_parser p(str);
		i64 total_ns = 0;

		while (!p.eof()) {
			p.skip_whitespace();
			if (p.eof()) {
				break;
			}

			i64 value = p.parse_int();
			p.skip_whitespace();

			string_view unit = p.parse_alpha();

			bool matched = false;
			for (auto& u : unit_trait<timespan>::units) {
				if (u.second == unit) {
					total_ns += value * u.first;
					matched = true;
					break;
				}
			}

			if (!matched) {
				throw std::runtime_error(lf::format("unknown time unit '{}' in '{}'", unit, str));
			}
		}

		return timespan(total_ns);
	}
} // namespace lf
