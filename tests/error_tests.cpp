#include <catch2/catch_test_macros.hpp>

#include "leaf/core/error.hpp"

/// Verifies that a default error is a success value.
TEST_CASE("error default construction is success", "[error]") {
	lf::error err;
	REQUIRE_FALSE(static_cast<bool>(err));
	REQUIRE(err.message.empty());
	REQUIRE(err.code.value() == 0);

	REQUIRE_FALSE(static_cast<bool>(lf::error::no_error));
	REQUIRE(static_cast<bool>(lf::error::unknown_error));
}

/// Verifies that errors constructed from enums carry the right category and value.
TEST_CASE("error code enum integration", "[error]") {
	lf::error err(lf::generic_errc::parse_error, "bad token");
	REQUIRE(static_cast<bool>(err));
	REQUIRE(err.code == lf::make_error_code(lf::generic_errc::parse_error));
	REQUIRE(err.code.category().name() == lf::string("generic"));
	REQUIRE(err.code.value() == static_cast<i32>(lf::generic_errc::parse_error));
	REQUIRE(err.message == "bad token");

	lf::error gfx(lf::graphics_errc::device_lost);
	REQUIRE(gfx.code.category().name() == lf::string("graphics"));
	REQUIRE(gfx.code == lf::make_error_code(lf::graphics_errc::device_lost));
	REQUIRE(gfx.code != err.code);

	// Category messages come from the enum names.
	REQUIRE(err.code.message() == "parse_error");
	REQUIRE(gfx.code.message() == "device_lost");
}

/// Verifies that a text-only error falls back to the unknown generic code.
TEST_CASE("error from message uses unknown code", "[error]") {
	lf::error err("something broke");
	REQUIRE(static_cast<bool>(err));
	REQUIRE(err.code == lf::make_error_code(lf::generic_errc::unknown));
	REQUIRE(err.message == "something broke");
}

/// Verifies that add_context prepends context and chains.
TEST_CASE("error add_context chains context", "[error]") {
	lf::error err(lf::generic_errc::missing_field, "field 'id' missing");
	err.add_context("parsing prototype");
	REQUIRE(err.message == "parsing prototype\n -> field 'id' missing");

	err.add_context("loading mod 'base'");
	REQUIRE(err.message == "loading mod 'base'\n -> parsing prototype\n -> field 'id' missing");

	// Empty context is a no-op.
	lf::string before = err.message;
	err.add_context("");
	REQUIRE(err.message == before);

	// add_context returns a reference for chaining.
	lf::error chained = lf::error("inner").add_context("outer");
	REQUIRE(chained.message == "outer\n -> inner");
}

namespace {
	lf::report<i32> parse_positive(i32 value) {
		if (value < 0) {
			return lf::unexpected(lf::error(lf::generic_errc::input_error, "negative value"));
		}
		return value;
	}

	lf::result<i32> half_even(i32 value) {
		if (value % 2 != 0) {
			return lf::unexpected(lf::make_error_code(lf::generic_errc::input_error));
		}
		return value / 2;
	}
} // namespace

/// Verifies that report and result propagate values and errors.
TEST_CASE("report and result propagate values and errors", "[error]") {
	auto good = parse_positive(5);
	REQUIRE(good.has_value());
	REQUIRE(*good == 5);

	auto bad = parse_positive(-1);
	REQUIRE_FALSE(bad.has_value());
	REQUIRE(static_cast<bool>(bad.error()));
	REQUIRE(bad.error().code == lf::make_error_code(lf::generic_errc::input_error));
	REQUIRE(bad.error().message == "negative value");

	auto ok = half_even(8);
	REQUIRE(ok.has_value());
	REQUIRE(*ok == 4);

	auto odd = half_even(7);
	REQUIRE_FALSE(odd.has_value());
	REQUIRE(odd.error() == lf::make_error_code(lf::generic_errc::input_error));
}

namespace {
	lf::error fail_when(bool fail) {
		if (fail) {
			return lf::error(lf::generic_errc::unknown, "inner failure");
		}
		return lf::error::no_error;
	}

	lf::error run_step(bool fail, bool& reached_end) {
		IF_ERROR_RETURN_ERROR(fail_when(fail));
		reached_end = true;
		return lf::error::no_error;
	}
} // namespace

/// Verifies that IF_ERROR_RETURN_ERROR early-returns on failure only.
TEST_CASE("IF_ERROR_RETURN_ERROR early returns on failure", "[error]") {
	bool reached_end = false;
	lf::error err = run_step(true, reached_end);
	REQUIRE(static_cast<bool>(err));
	REQUIRE_FALSE(reached_end);
	REQUIRE(err.message == "inner failure");

	reached_end = false;
	err = run_step(false, reached_end);
	REQUIRE_FALSE(static_cast<bool>(err));
	REQUIRE(reached_end);
}
