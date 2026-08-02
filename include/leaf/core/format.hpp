#pragma once

#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace lf {
	namespace detail {
		inline void append_arg(std::string& out, std::string_view text) {
			out.append(text.data(), text.size());
		}

		inline void append_arg(std::string& out, const std::string& text) {
			out.append(text);
		}

		inline void append_arg(std::string& out, const char* text) {
			if (text) out.append(text);
		}

		inline void append_arg(std::string& out, char* text) {
			if (text) out.append(text);
		}

		inline void append_arg(std::string& out, bool value) {
			out.append(value ? "true" : "false");
		}

		template<typename T>
		inline void append_arg(std::string& out, const T& value) {
			if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>) {
				out.append(std::to_string(value));
			} else if constexpr (std::is_enum_v<T>) {
				using U = std::underlying_type_t<T>;
				out.append(std::to_string(static_cast<U>(value)));
			} else {
				std::ostringstream stream;
				stream << value;
				out.append(stream.str());
			}
		}

		inline void format_impl(std::string& out, std::string_view pattern, std::size_t pos) {
			out.append(pattern.substr(pos));
		}

		template<typename Arg, typename... Args>
		void format_impl(std::string& out, std::string_view pattern, std::size_t pos, Arg&& arg, Args&&... args) {
			const std::size_t open = pattern.find('{', pos);
			if (open == std::string_view::npos) {
				out.append(pattern.substr(pos));
				return;
			}

			const std::size_t close = pattern.find('}', open + 1);
			if (close == std::string_view::npos) {
				out.append(pattern.substr(pos));
				return;
			}

			out.append(pattern.substr(pos, open - pos));
			append_arg(out, std::forward<Arg>(arg));
			format_impl(out, pattern, close + 1, std::forward<Args>(args)...);
		}
	} // namespace detail

	template<typename... Args>
	std::string format(std::string_view pattern, Args&&... args) {
		std::string out;
		out.reserve(pattern.size() + sizeof...(Args) * 8);
		detail::format_impl(out, pattern, 0, std::forward<Args>(args)...);
		return out;
	}
} // namespace lf
