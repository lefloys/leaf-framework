#include <catch2/catch_test_macros.hpp>

#include <leaf/core/binary.hpp>
#include <leaf/core/random.hpp>

#include <limits>

namespace leaf_test::binary {
	inline constexpr lf::version schema_version = lf::version(1, 0, 0, 0);

	struct narrow_urbg {
		using result_type = u08;

		static constexpr result_type min() { return 0; }
		static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }

		result_type operator()() {
			++calls;
			return next++;
		}

		u08 next = 1;
		size_t calls = 0;
	};

	struct version_context {
		bool called = false;
	};

	struct custom_record {
		u32 value = 0;
	};

	struct fallback_record {
		u32 value = 0;
	};

	struct wrong_result_record {
		u32 value = 0;
	};

	template<lf::bin::byte_stream Stream>
	lf::error process(Stream& stream, custom_record& value, lf::schema_version<schema_version>, version_context& context) {
		context.called = true;
		u32 wire_value = value.value + 1;
		return stream(lf::field("custom", wire_value));
	}

	template<lf::bin::byte_stream Stream>
	bool process(Stream&, wrong_result_record&, lf::schema_version<schema_version>) {
		return true;
	}

	template<typename T>
	lf::report<lf::vector<lf::byte>> write_versioned(T& value) {
		lf::bin::write_stream stream;
		if (lf::error err = stream(lf::field("value", value, lf::schema_version<schema_version>{}))) {
			return lf::unexpected(std::move(err));
		}
		return stream.take_written();
	}
} // namespace leaf_test::binary

template<>
struct lf::schema_trait<leaf_test::binary::custom_record, leaf_test::binary::schema_version> {
	static auto get(leaf_test::binary::custom_record& value) { return lf::group(lf::field("fallback", value.value)); }
};

template<>
struct lf::schema_trait<leaf_test::binary::fallback_record, leaf_test::binary::schema_version> {
	static auto get(leaf_test::binary::fallback_record& value) { return lf::group(lf::field("fallback", value.value)); }
};

template<>
struct lf::schema_trait<leaf_test::binary::wrong_result_record, leaf_test::binary::schema_version> {
	static auto get(leaf_test::binary::wrong_result_record& value) { return lf::group(lf::field("fallback", value.value)); }
};

TEST_CASE("random seed consumes the full URBG result width") {
	leaf_test::binary::narrow_urbg generator;
	const u64 seed = lf::detail::random_seed(generator);

	REQUIRE(generator.calls >= sizeof(u64));
	REQUIRE((seed >> 32u) != 0);
}

TEST_CASE("versioned binary processing uses a compatible ADL processor or schema fallback") {
	leaf_test::binary::custom_record custom{41};
	leaf_test::binary::version_context context;
	lf::bin::write_stream custom_stream;
	REQUIRE_FALSE(custom_stream(lf::field("value", custom, lf::schema_version<leaf_test::binary::schema_version>{}, context)));
	const auto custom_bytes = custom_stream.take_written();
	REQUIRE(context.called);
	REQUIRE(custom_bytes.size() == sizeof(u32));

	leaf_test::binary::fallback_record fallback{17};
	auto fallback_bytes = leaf_test::binary::write_versioned(fallback);
	REQUIRE(fallback_bytes.has_value());
	REQUIRE(fallback_bytes->size() == sizeof(u32));

	leaf_test::binary::wrong_result_record wrong_result{23};
	auto wrong_result_bytes = leaf_test::binary::write_versioned(wrong_result);
	REQUIRE(wrong_result_bytes.has_value());
	REQUIRE(wrong_result_bytes->size() == sizeof(u32));
}
