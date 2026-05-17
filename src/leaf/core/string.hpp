#pragma once
#include "typename.hpp"
#include "types.hpp"

#include <string>
#include <string_view>

namespace lf {
	using string = std::basic_string<char>;
	using wstring = std::basic_string<wchar>;
	using string_view = std::basic_string_view<char>;
	using wstring_view = std::basic_string_view<wchar>;

	template <>
	struct lf::type_name_trait<string> {
		static constexpr const char* get() {
			return "string";
		}
	};
	template <>
	struct lf::type_name_trait<wstring> {
		static constexpr const char* get() {
			return "wstring";
		}
	};
	template <>
	struct lf::type_name_trait<string_view> {
		static constexpr const char* get() {
			return "string_view";
		}
	};
	template <>
	struct lf::type_name_trait<wstring_view> {
		static constexpr const char* get() {
			return "wstring_view";
		}
	};

	struct string_parser {
		string_view str;
		size_t pos = 0;

		explicit string_parser(string_view s);

		bool eof() const;
		char peek() const;

		void skip_whitespace();
		bool consume(string_view expected);

		i64 parse_int();
		string_view parse_alpha();
		i64 parse_fraction(i64& digits);
	};

	template <typename T>
	struct pretty_string_trait {
		static string to_string(const T& value) = delete;
		static T from_string(string_view str) = delete;
	};
	template <typename T>
	string to_pretty_string(const T& value) {
		return pretty_string_trait<T>::to_string(value);
	}
	template <typename T>
	T from_pretty_string(string_view str) {
		return pretty_string_trait<T>::from_string(str);
	}
} // namespace lf