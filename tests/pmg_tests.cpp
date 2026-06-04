#include <cstring>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "leaf/pmg/pmg.hpp"

struct TestVertex {
	u32 id = 0;
	f32 x = 0.0f;
	f32 y = 0.0f;
};

const TestVertex* as_vertices(const lf::vector<u08>& bytes) {
	return reinterpret_cast<const TestVertex*>(bytes.data());
}

const u32* as_indices(const lf::vector<u08>& bytes) {
	return reinterpret_cast<const u32*>(bytes.data());
}

/// Verifies that pmg non-indexed writer stores new vertex bytes.
TEST_CASE("pmg non-indexed writer stores new vertex bytes", "[pmg]") {
	lf::vector<u08> vertices;
	lf::pmg::writer<TestVertex> writer(vertices);
	lf::pmg::count_source source{ .count = 2 };

	u32 next_id = 1;
	for (auto& tri : lf::pmg::triangles<TestVertex>(writer, source)) {
		tri.v0().id = next_id++;
		tri.v1().id = next_id++;
		tri.v2().id = next_id++;
	}

	REQUIRE(vertices.size() == 6 * sizeof(TestVertex));
	const TestVertex* written = as_vertices(vertices);
	REQUIRE(written[0].id == 1);
	REQUIRE(written[5].id == 6);
}

/// Verifies that pmg non-indexed repush copies previous vertex bytes.
TEST_CASE("pmg non-indexed repush copies previous vertex bytes", "[pmg]") {
	lf::vector<u08> vertices;
	lf::pmg::writer<TestVertex> writer(vertices);
	lf::pmg::count_source source{ .count = 2 };

	u32 triangle = 0;
	for (auto& tri : lf::pmg::triangles<TestVertex>(writer, source)) {
		if (triangle == 0) {
			tri.v0().id = 10;
			tri.v1().id = 11;
			tri.v2().id = 12;
		} else {
			tri.repush_v0(0);
			tri.repush_v1(1);
			tri.repush_v2(2);
		}
		++triangle;
	}

	const TestVertex* written = as_vertices(vertices);
	REQUIRE(written[3].id == 12);
	REQUIRE(written[4].id == 11);
	REQUIRE(written[5].id == 10);
}

/// Verifies that pmg indexed writer stores sequential indices.
TEST_CASE("pmg indexed writer stores sequential indices", "[pmg]") {
	lf::vector<u08> vertices;
	lf::vector<u08> indices;
	lf::pmg::indexed_writer<TestVertex> writer(vertices, indices);
	lf::pmg::count_source source{ .count = 1 };

	for (auto& tri : lf::pmg::triangles<TestVertex>(writer, source)) {
		tri.v0().id = 20;
		tri.v1().id = 21;
		tri.v2().id = 22;
	}

	REQUIRE(vertices.size() == 3 * sizeof(TestVertex));
	REQUIRE(indices.size() == 3 * sizeof(u32));
	const u32* written_indices = as_indices(indices);
	REQUIRE(written_indices[0] == 0);
	REQUIRE(written_indices[1] == 1);
	REQUIRE(written_indices[2] == 2);
}

/// Verifies that pmg indexed repush emits indices without duplicating vertices.
TEST_CASE("pmg indexed repush emits indices without duplicating vertices", "[pmg]") {
	lf::vector<u08> vertices;
	lf::vector<u08> indices;
	lf::pmg::indexed_writer<TestVertex> writer(vertices, indices);
	lf::pmg::count_source source{ .count = 2 };

	u32 triangle = 0;
	for (auto& tri : lf::pmg::triangles<TestVertex>(writer, source)) {
		if (triangle == 0) {
			tri.v0().id = 30;
			tri.v1().id = 31;
			tri.v2().id = 32;
		} else {
			tri.repush_v0(0);
			tri.repush_v1(1);
			tri.repush_v2(2);
		}
		++triangle;
	}

	REQUIRE(vertices.size() == 3 * sizeof(TestVertex));
	REQUIRE(indices.size() == 6 * sizeof(u32));
	const u32* written_indices = as_indices(indices);
	REQUIRE(written_indices[3] == 2);
	REQUIRE(written_indices[4] == 1);
	REQUIRE(written_indices[5] == 0);
}

/// Verifies that pmg auto-flush preserves order when staging overflows.
TEST_CASE("pmg auto-flush preserves order when staging overflows", "[pmg]") {
	lf::vector<u08> vertices;
	lf::pmg::writer<TestVertex> writer(vertices, { .staging_capacity = 3 * sizeof(TestVertex) });
	lf::pmg::count_source source{ .count = 3 };

	u32 next_id = 1;
	for (auto& tri : lf::pmg::triangles<TestVertex>(writer, source)) {
		tri.v0().id = next_id++;
		tri.v1().id = next_id++;
		tri.v2().id = next_id++;
	}

	const TestVertex* written = as_vertices(vertices);
	for (u32 i = 0; i < 9; ++i) {
		REQUIRE(written[i].id == i + 1);
	}
}

/// Verifies that pmg repush outside history throws.
TEST_CASE("pmg repush outside history throws", "[pmg]") {
	lf::vector<u08> vertices;
	lf::pmg::writer<TestVertex> writer(vertices);
	lf::pmg::count_source source{ .count = 1 };

	REQUIRE_THROWS_AS(([&] {
						  for (auto& tri : lf::pmg::triangles<TestVertex>(writer, source)) {
							  tri.repush_v0(0);
						  }
					  }()),
					  lf::out_of_range_exception);
}

/// Verifies that pmg thread-local staging is isolated per thread.
TEST_CASE("pmg thread-local staging is isolated per thread", "[pmg]") {
	lf::vector<u08> first;
	lf::vector<u08> second;

	auto generate = [](lf::vector<u08>& out, u32 base) {
		lf::pmg::writer<TestVertex> writer(out);
		lf::pmg::count_source source{ .count = 1 };
		for (auto& tri : lf::pmg::triangles<TestVertex>(writer, source)) {
			tri.v0().id = base;
			tri.v1().id = base + 1;
			tri.v2().id = base + 2;
		}
	};

	std::thread a(generate, std::ref(first), 100);
	std::thread b(generate, std::ref(second), 200);
	a.join();
	b.join();

	REQUIRE(as_vertices(first)[0].id == 100);
	REQUIRE(as_vertices(second)[0].id == 200);
}
