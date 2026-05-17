#include "string.hpp"
#include "exception.hpp"

namespace lf {
	string_parser::string_parser(string_view s) : str(s) {}

	bool string_parser::eof() const {
		return pos >= str.size();
	}

	char string_parser::peek() const {
		if (eof()) {
			return '\0';
		}
		return str[pos];
	}

	void string_parser::skip_whitespace() {
		while (!eof() && std::isspace(peek())) {
			++pos;
		}
	}

	bool string_parser::consume(string_view expected) {
		if (str.substr(pos, expected.size()) == expected) {
			pos += expected.size();
			return true;
		}
		return false;
	}

	i64 string_parser::parse_int() {
		if (!std::isdigit(peek())) {
			throw runtime_exception("expected number");
		}

		i64 val = 0;
		while (!eof() && std::isdigit(peek())) {
			val = val * 10 + (peek() - '0');
			++pos;
		}

		return val;
	}

	string_view string_parser::parse_alpha() {
		size_t start = pos;

		while (!eof() && std::isalpha(peek())) {
			++pos;
		}

		if (start == pos) {
			throw runtime_exception("expected unit");
		}

		return str.substr(start, pos - start);
	}

	i64 string_parser::parse_fraction(i64& digits) {
		i64 frac = 0;
		digits = 0;

		while (!eof() && std::isdigit(peek())) {
			if (digits < 9) {
				frac = frac * 10 + (peek() - '0');
				++digits;
			}
			++pos;
		}

		return frac;
	}

} // namespace lf