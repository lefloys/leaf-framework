#include <catch2/catch_test_macros.hpp>

#include "leaf/math/dim.hpp"
#include "leaf/math/pos.hpp"
#include "leaf/math/rect.hpp"
#include "leaf/math/vec.hpp"

/// Verifies that vec aliases behave like glm vectors with leaf scalar types.
TEST_CASE("vec aliases support arithmetic", "[math]") {
	lf::vec2<f32> a{ 1.0f, 2.0f };
	lf::vec2<f32> b{ 3.0f, 4.0f };
	lf::vec2<f32> sum = a + b;
	REQUIRE(sum.x == 4.0f);
	REQUIRE(sum.y == 6.0f);

	lf::vec3<i32> v3{ 1, 2, 3 };
	lf::vec3<i32> scaled = v3 * 2;
	REQUIRE(scaled.x == 2);
	REQUIRE(scaled.y == 4);
	REQUIRE(scaled.z == 6);

	lf::vec4<f64> v4{ 1.0, 2.0, 3.0, 4.0 };
	lf::vec4<f64> diff = v4 - lf::vec4<f64>{ 0.5, 0.5, 0.5, 0.5 };
	REQUIRE(diff.x == 0.5);
	REQUIRE(diff.w == 3.5);

	REQUIRE(a == lf::vec2<f32>{ 1.0f, 2.0f });
	REQUIRE(a != b);
}

/// Verifies that pos2 and dim2 default construct to zero.
TEST_CASE("pos2 and dim2 value initialize", "[math]") {
	lf::pos2<i32> p;
	REQUIRE(p.x == 0);
	REQUIRE(p.y == 0);

	lf::dim2<f32> d;
	REQUIRE(d.width == 0.0f);
	REQUIRE(d.height == 0.0f);

	lf::pos2<i32> placed{ 10, -5 };
	REQUIRE(placed.x == 10);
	REQUIRE(placed.y == -5);

	lf::dim2<u32> sized{ 1920u, 1080u };
	REQUIRE(sized.width == 1920u);
	REQUIRE(sized.height == 1080u);
}

/// Verifies that rect aggregates position and dimensions.
TEST_CASE("rect aggregates pos and dim", "[math]") {
	lf::rect<i32> r{ { 4, 8 }, { 100, 50 } };
	REQUIRE(r.pos.x == 4);
	REQUIRE(r.pos.y == 8);
	REQUIRE(r.dim.width == 100);
	REQUIRE(r.dim.height == 50);

	lf::rect<f32> empty;
	REQUIRE(empty.pos.x == 0.0f);
	REQUIRE(empty.pos.y == 0.0f);
	REQUIRE(empty.dim.width == 0.0f);
	REQUIRE(empty.dim.height == 0.0f);

	// Derived edges from the aggregate.
	REQUIRE(r.pos.x + r.dim.width == 104);
	REQUIRE(r.pos.y + r.dim.height == 58);
}
