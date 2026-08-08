#include <catch2/catch_test_macros.hpp>

#include <leaf/core/time.hpp>

#include <chrono>
#include <type_traits>

static_assert(!std::is_same_v<lf::instant, lf::duration>);
static_assert(!std::is_same_v<lf::instant, lf::timespan>);
static_assert(!std::is_same_v<lf::duration, lf::timespan>);
static_assert(!std::is_convertible_v<lf::instant, lf::duration>);
static_assert(!std::is_convertible_v<lf::duration, lf::timespan>);

TEST_CASE("time types preserve point and duration geometry") {
	const lf::duration vector = lf::duration::from_chrono(std::chrono::milliseconds(250));
	const lf::instant point = lf::instant::from_quantum(1'000'000'000);
	const lf::instant advanced = point + vector;

	REQUIRE(vector.to_chrono<i64, std::milli>().count() == 250);
	REQUIRE((advanced - point) == vector);
	REQUIRE(lf::timespan::from_quantum(250'000'000).quantum_count() == vector.quantum_count());
}

TEST_CASE("frequency preserves integral and fractional hertz values") {
	const lf::frequency integral = lf::frequency::from_hertz(60);

	REQUIRE(integral.hertz().raw() == 60 * lf::fixed::scale);
	REQUIRE(lf::frequency::from_hertz(2.5).hertz_value() == 2.5);
}
