#include <catch2/catch_test_macros.hpp>

#include "leaf/core/exception.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/time.hpp"

/// Verifies that string_parser consumes tokens and tracks eof.
TEST_CASE("string_parser consumes tokens", "[string]") {
	lf::string_parser p("  42abc");
	REQUIRE_FALSE(p.eof());
	p.skip_whitespace();
	REQUIRE(p.peek() == '4');
	REQUIRE(p.parse_int() == 42);
	REQUIRE(p.parse_alpha() == "abc");
	REQUIRE(p.eof());
	REQUIRE(p.peek() == '\0');
}

/// Verifies that string_parser consume matches literal prefixes.
TEST_CASE("string_parser consume matches literals", "[string]") {
	lf::string_parser p("key=value");
	REQUIRE_FALSE(p.consume("value"));
	REQUIRE(p.consume("key"));
	REQUIRE(p.consume("="));
	REQUIRE(p.consume("value"));
	REQUIRE(p.eof());
}

/// Verifies that string_parser throws on malformed numbers and units.
TEST_CASE("string_parser rejects malformed input", "[string]") {
	lf::string_parser p("abc");
	REQUIRE_THROWS(p.parse_int());

	lf::string_parser q("123");
	REQUIRE_THROWS(q.parse_alpha());
}

/// Verifies that string_parser parse_fraction caps at nine digits.
TEST_CASE("string_parser parse_fraction caps digits", "[string]") {
	lf::string_parser p("25");
	i64 digits = 0;
	REQUIRE(p.parse_fraction(digits) == 25);
	REQUIRE(digits == 2);

	lf::string_parser q("1234567890123");
	digits = 0;
	REQUIRE(q.parse_fraction(digits) == 123456789);
	REQUIRE(digits == 9);
	REQUIRE(q.eof());
}

/// Verifies that timespan pretty printing decomposes into units.
TEST_CASE("timespan to_pretty_string decomposes units", "[time]") {
	constexpr i64 second = 1'000'000'000LL;
	REQUIRE(lf::to_pretty_string(lf::timespan::from_quantum(0)) == "0s");
	REQUIRE(lf::to_pretty_string(lf::timespan::from_quantum(second)) == "1s");
	REQUIRE(lf::to_pretty_string(lf::timespan::from_quantum(90 * second)) == "1m 30s");
	REQUIRE(lf::to_pretty_string(lf::timespan::from_quantum(
				(26 * 3600 + 3 * 60 + 4) * second))
			== "1d 2h 3m 4s");
}

/// Verifies that timespan parsing round trips through pretty strings.
TEST_CASE("timespan from_pretty_string round trips", "[time]") {
	constexpr i64 second = 1'000'000'000LL;
	REQUIRE(lf::from_pretty_string<lf::timespan>("1s").quantum_count() == second);
	REQUIRE(lf::from_pretty_string<lf::timespan>("1m 30s").quantum_count() == 90 * second);

	const lf::timespan original = lf::timespan::from_quantum((26 * 3600 + 3 * 60 + 4) * second);
	const lf::string text = lf::to_pretty_string(original);
	REQUIRE(lf::from_pretty_string<lf::timespan>(text).quantum_count() == original.quantum_count());
}

/// Verifies that instant and duration arithmetic is consistent.
TEST_CASE("instant and duration arithmetic", "[time]") {
	const lf::instant start = lf::instant::from_quantum(1'000);
	const lf::duration step = lf::duration::from_quantum(250);

	const lf::instant later = start + step;
	REQUIRE(later.quantum_count() == 1'250);
	REQUIRE((later - step).quantum_count() == start.quantum_count());
	REQUIRE(later.quantum_count() - start.quantum_count() == step.quantum_count());

	lf::duration accumulated = step;
	accumulated += step;
	REQUIRE(accumulated.quantum_count() == 500);
	accumulated -= lf::duration::from_quantum(100);
	REQUIRE(accumulated.quantum_count() == 400);
	REQUIRE((-step).quantum_count() == -250);
}
