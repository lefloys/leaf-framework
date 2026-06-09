#pragma once

#include "exception.hpp"
#include "string.hpp"

#include <unordered_map>

namespace lf {
	template <typename kT, typename vT, typename Hasher = std::hash<kT>, typename KeyEqual = std::equal_to<kT>>
	class unordered_map : public std::unordered_map<kT, vT, Hasher, KeyEqual> {
	  public:
		vT& at(const kT& key) {
			auto it = this->find(key);
			if (it == this->end()) {
				throw out_of_range_exception(format("key '{}' not found", key));
			}
			return it->second;
		}
		const vT& at(const kT& key) const {
			auto it = this->find(key);
			if (it == this->end()) {
				throw out_of_range_exception(format("key '{}' not found", key));
			}
			return it->second;
		}
	};

	struct transparent_string_view_hash {
		using is_transparent = void;

		size_t operator()(string_view sv) const noexcept { return std::hash<string_view>{}(sv); }
		size_t operator()(const std::string& s) const noexcept { return std::hash<std::string_view>{}(s); }
		size_t operator()(const char* s) const noexcept { return std::hash<std::string_view>{}(s); }
	};

	struct transparent_string_view_equal {
		using is_transparent = void;
		bool operator()(string_view lhs, string_view rhs) const noexcept {
			return lhs == rhs;
		}
	};

	template <typename T>
	using unordered_map_string = unordered_map<string, T, transparent_string_view_hash, transparent_string_view_equal>;
} // namespace lf