#include <catch2/catch_test_macros.hpp>

#include "leaf/core/fixed.hpp"
#include "leaf/script/fixed_script.hpp"

#include <sol/sol.hpp>

/// Verifies that fixed parses exact decimal values.
TEST_CASE("fixed parses exact decimal values", "[fixed]") {
	auto zero = lf::fixed::parse("0");
	auto one = lf::fixed::parse("1");
	auto negative_one = lf::fixed::parse("-1");
	auto one_and_quarter = lf::fixed::parse("1.25");
	auto quantum = lf::fixed::parse("0.000000001");
	REQUIRE(zero.has_value());
	REQUIRE(one.has_value());
	REQUIRE(negative_one.has_value());
	REQUIRE(one_and_quarter.has_value());
	REQUIRE(quantum.has_value());
	REQUIRE(zero->raw() == 0);
	REQUIRE(one->raw() == lf::fixed::scale);
	REQUIRE(negative_one->raw() == -lf::fixed::scale);
	REQUIRE(one_and_quarter->raw() == 1'250'000'000);
	REQUIRE(quantum->raw() == 1);
	REQUIRE(lf::fixed::parse("0.0000000001").has_value() == false);
}

/// Verifies that fixed detects overflow and formats round trips.
TEST_CASE("fixed detects overflow and formats round trips", "[fixed]") {
	auto max = lf::fixed::parse("9223372036.854775807");
	REQUIRE(max.has_value());
	REQUIRE(max->raw() == 9'223'372'036'854'775'807ll);
	REQUIRE_FALSE(lf::fixed::parse("9223372036.854775808").has_value());

	auto parsed_value = lf::fixed::parse("-120.340000005");
	REQUIRE(parsed_value.has_value());
	lf::fixed value = *parsed_value;
	REQUIRE(value.to_string() == "-120.340000005");
	auto round_tripped = lf::fixed::parse(value.to_string());
	REQUIRE(round_tripped.has_value());
	REQUIRE(round_tripped->raw() == value.raw());
}

/// Verifies that fixed arithmetic is exact.
TEST_CASE("fixed arithmetic is exact", "[fixed]") {
	auto one_parsed = lf::fixed::parse("1");
	auto quarter_parsed = lf::fixed::from_ratio(1, 4);
	auto half_parsed = lf::fixed::parse("0.5");
	auto four_parsed = lf::fixed::from_integer(4);
	REQUIRE(one_parsed.has_value());
	REQUIRE(quarter_parsed.has_value());
	REQUIRE(half_parsed.has_value());
	REQUIRE(four_parsed.has_value());
	lf::fixed one = *one_parsed;
	lf::fixed quarter = *quarter_parsed;
	lf::fixed half = *half_parsed;

	REQUIRE((one + quarter).to_string() == "1.25");
	REQUIRE((one - quarter).to_string() == "0.75");
	REQUIRE((half * half).to_string() == "0.25");
	REQUIRE((one / *four_parsed).to_string() == "0.25");
}

/// Verifies that fixed Lua binding arithmetic and coercion.
TEST_CASE("fixed Lua binding arithmetic and coercion", "[fixed][lua]") {
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::table);
	lf::InstallFixedScript(lua, true);

	lua.script(R"lua(
sum = fixed("1.25") + fixed_ratio(1, 4)
ordered = fixed("1.25") < fixed("1.5")
global_value = 1.25
local local_value = 1.25
local_type = type(local_value)
)lua");

	REQUIRE(lua["sum"].get<lf::fixed>().to_string() == "1.5");
	REQUIRE(lua["ordered"].get<bool>());
	REQUIRE(lua["global_value"].get<lf::fixed>().to_string() == "1.25");
	REQUIRE(lua["local_type"].get<lf::string>() == "number");
}
