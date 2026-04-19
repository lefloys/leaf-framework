#include <leaf/core/iterator.hpp>
#include <leaf/core/cstddef.hpp>
#include <leaf/core/vector.hpp>

#include <catch2/catch_test_macros.hpp>

namespace {
	template<typename Range>
	lf::vector<typename Range::iterator::value_type> collect_values(const Range& range) {
		lf::vector<typename Range::iterator::value_type> values;
		for (auto value : range) {
			values.push_back(value);
		}

		return values;
	}
}

TEST_CASE("range handles empty positive ranges", "[iterator][range]") {
	const lf::vector<lf::size_t> values = collect_values(lf::range(0, 0));
	REQUIRE(values.empty());
}

TEST_CASE("range handles one-element ranges", "[iterator][range]") {
	const lf::vector<lf::size_t> values = collect_values(lf::range(0, 1));
	REQUIRE(values == lf::vector<lf::size_t>{ 0 });
}

TEST_CASE("range handles many elements", "[iterator][range]") {
	const lf::vector<lf::size_t> values = collect_values(lf::range(0, 5));
	REQUIRE(values == lf::vector<lf::size_t>{ 0, 1, 2, 3, 4 });
}

TEST_CASE("range with step matches python-style positive behavior", "[iterator][range]") {
	REQUIRE(collect_values(lf::range(0, 1, 2)) == lf::vector<lf::size_t>{ 0 });
	REQUIRE(collect_values(lf::range(0, 6, 2)) == lf::vector<lf::size_t>{ 0, 2, 4 });
	REQUIRE(collect_values(lf::range(5, 5, 2)).empty());
}

TEST_CASE("rrange handles empty ranges", "[iterator][rrange]") {
	const lf::vector<lf::size_t> values = collect_values(lf::rrange(0, 0));
	REQUIRE(values.empty());
}

TEST_CASE("rrange handles one-element ranges", "[iterator][rrange]") {
	const lf::vector<lf::size_t> values = collect_values(lf::rrange(0, 1));
	REQUIRE(values == lf::vector<lf::size_t>{ 0 });
}

TEST_CASE("rrange handles many elements", "[iterator][rrange]") {
	const lf::vector<lf::size_t> values = collect_values(lf::rrange(0, 5));
	REQUIRE(values == lf::vector<lf::size_t>{ 4, 3, 2, 1, 0 });
}

TEST_CASE("rrange handles stepped reverse iteration", "[iterator][rrange]") {
	REQUIRE(collect_values(lf::rrange(0, 1, 2)) == lf::vector<lf::size_t>{ 0 });
	REQUIRE(collect_values(lf::rrange(0, 6, 2)) == lf::vector<lf::size_t>{ 4, 2, 0 });
	REQUIRE(collect_values(lf::rrange(5, 5, 2)).empty());
}

TEST_CASE("enumerate handles empty vectors", "[iterator][enumerate]") {
	lf::vector<int> values;
	lf::vector<lf::size_t> indices;

	for (auto [index, value] : lf::enumerate(values)) {
		indices.push_back(index);
		indices.push_back(static_cast<lf::size_t>(value));
	}

	REQUIRE(indices.empty());
}

TEST_CASE("enumerate handles one element", "[iterator][enumerate]") {
	lf::vector<int> values{ 7 };
	lf::vector<lf::size_t> indices;
	lf::vector<int> enumerated_values;

	for (auto [index, value] : lf::enumerate(values)) {
		indices.push_back(index);
		enumerated_values.push_back(value);
	}

	REQUIRE(indices == lf::vector<lf::size_t>{ 0 });
	REQUIRE(enumerated_values == lf::vector<int>{ 7 });
}

TEST_CASE("enumerate handles many elements", "[iterator][enumerate]") {
	lf::vector<int> values{ 4, 8, 15, 16, 23, 42 };
	lf::vector<lf::size_t> indices;
	lf::vector<int> enumerated_values;

	for (auto [index, value] : lf::enumerate(values)) {
		indices.push_back(index);
		enumerated_values.push_back(value);
	}

	REQUIRE(indices == lf::vector<lf::size_t>{ 0, 1, 2, 3, 4, 5 });
	REQUIRE(enumerated_values == values);
}

TEST_CASE("renumerate handles empty vectors", "[iterator][renumerate]") {
	lf::vector<int> values;
	lf::vector<lf::size_t> indices;

	for (auto [index, value] : lf::renumerate(values)) {
		indices.push_back(index);
		indices.push_back(static_cast<lf::size_t>(value));
	}

	REQUIRE(indices.empty());
}

TEST_CASE("renumerate handles one element", "[iterator][renumerate]") {
	lf::vector<int> values{ 7 };
	lf::vector<lf::size_t> indices;
	lf::vector<int> enumerated_values;

	for (auto [index, value] : lf::renumerate(values)) {
		indices.push_back(index);
		enumerated_values.push_back(value);
	}

	REQUIRE(indices == lf::vector<lf::size_t>{ 0 });
	REQUIRE(enumerated_values == lf::vector<int>{ 7 });
}

TEST_CASE("renumerate handles many elements", "[iterator][renumerate]") {
	lf::vector<int> values{ 4, 8, 15, 16 };
	lf::vector<lf::size_t> indices;
	lf::vector<int> enumerated_values;

	for (auto [index, value] : lf::renumerate(values)) {
		indices.push_back(index);
		enumerated_values.push_back(value);
	}

	REQUIRE(indices == lf::vector<lf::size_t>{ 3, 2, 1, 0 });
	REQUIRE(enumerated_values == lf::vector<int>{ 16, 15, 8, 4 });
}
