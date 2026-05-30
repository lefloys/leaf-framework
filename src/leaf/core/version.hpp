#pragma once
#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"

namespace lf {
	struct version {
		u16 major;
		u16 minor;
		u16 patch;
		u16 snapshot;

		constexpr version(u16 maj = 0, u16 min = 0, u16 pat = 0, u16 snap = 0)
			: major(maj), minor(min), patch(pat), snapshot(snap) {}

		constexpr bool operator==(const version&) const = default;

		static version from_string(string_view str) {
			version ver;
			auto dash_pos = str.find('-');
			string_view main = str;
			string_view snap;
			if (dash_pos != str.npos) {
				main = str.substr(0, dash_pos);
				snap = str.substr(dash_pos + 1);
			}
			size_t start = 0;
			int idx = 0;
			while (start < main.size() && idx < 3) {
				auto dot = main.find('.', start);
				auto end = (dot == main.npos) ? main.size() : dot;
				auto part = main.substr(start, end - start);
				// Only parse if all characters are digits
				bool numeric = true;
				for (auto c : part) {
					if (c < '0' || c > '9') {
						numeric = false;
						break;
					}
				}
				u16 val = 0;
				if (numeric && !part.empty()) {
					val = static_cast<u16>(std::atoi(part.data()));
				}
				if (idx == 0) {
					ver.major = val;
				} else if (idx == 1) {
					ver.minor = val;
				} else if (idx == 2) {
					ver.patch = val;
				}
				idx++;
				start = end + 1;
			}
			if (!snap.empty()) {
				bool numeric = true;
				for (auto c : snap) {
					if (c < '0' || c > '9') {
						numeric = false;
						break;
					}
				}
				if (numeric) {
					ver.snapshot = static_cast<u16>(std::atoi(snap.data()));
				}
			}
			return ver;
		}
	};
} // namespace lf
