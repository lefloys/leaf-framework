#include <concepts>
#include <array>
#include <cstddef>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "leaf/core/binary.hpp"

struct BinaryPlayer {
	lf::vec2<f32> position{};
	lf::string name;
	i32 health = 0;
};

struct BinaryTrivialRecord {
	u32 id = 0;
	i32 x = 0;
	i32 y = 0;
	f32 weight = 0.0f;
};

bool operator==(const BinaryTrivialRecord& lhs, const BinaryTrivialRecord& rhs) {
	return lhs.id == rhs.id
		&& lhs.x == rhs.x
		&& lhs.y == rhs.y
		&& lhs.weight == rhs.weight;
}

template<>
constexpr bool lf::bin::trivially_binary_serializable_v<BinaryTrivialRecord> = true;

bool operator==(const BinaryPlayer& lhs, const BinaryPlayer& rhs) {
	return lhs.position.x == rhs.position.x
		&& lhs.position.y == rhs.position.y
		&& lhs.name == rhs.name
		&& lhs.health == rhs.health;
}

struct BinaryVersionedValue {
	i32 value = 0;
	i32 extra = 0;
};

struct BinaryMissingSourceValue {
	i32 value = 0;
};

struct BinaryMissingMigrationValue {
	i32 value = 0;
};

enum class BinaryEnum : u16 {
	Alpha = 0x1234u,
	Beta = 0xabcdu
};

enum class BinaryValidatedEnum : u08 {
	Known = 1
};

enum class BinaryTestMode : u08 {
	None = 0,
	Full = 1,
	Compact = 2
};

template<>
struct lf::is_bitfield_enum<BinaryTestMode> : std::true_type {};

struct BinaryModeStream {
	using stream_tag = lf::bin::write_stream_tag;

	lf::bin::write_stream inner;
	BinaryTestMode current_mode = BinaryTestMode::None;

	BinaryTestMode mode() const { return current_mode; }
	lf::error bytes(lf::span<const lf::byte> in) { return inner.bytes(in); }
	lf::error bytes(const void* in, size_t size) { return inner.bytes(in, size); }
	template<typename T> lf::error write_scalar(const T& value) { return inner.write_scalar(value); }
	lf::error padding(size_t size) { return inner.padding(size); }
	const lf::string& context() const { return inner.context(); }
	void set_context(lf::string context) { inner.set_context(std::move(context)); }
	template<typename T> lf::error process(T& value) { return inner.process(value); }
	template<typename T> lf::error process(const T& value) { return inner.process(value); }
	template<lf::bin::binary_field... Fields> lf::error operator()(Fields&&... fields) {
		return lf::bin::detail::process_fields(*this, "writing", std::forward<Fields>(fields)...);
	}
};

struct BinaryPolyBase {
	virtual ~BinaryPolyBase() = default;
	virtual u08 type_id() const = 0;
	i32 value = 0;
};

struct BinaryPolyA : BinaryPolyBase {
	BinaryPolyA() { value = 11; }
	u08 type_id() const override { return 1; }
};

struct BinaryPolyB : BinaryPolyBase {
	BinaryPolyB() { value = 22; }
	u08 type_id() const override { return 2; }
};

struct BinaryPolyHolder {
	lf::unique_ptr<BinaryPolyBase> item;
};

struct BinaryGraphNode {
	i32 value = 0;
	lf::bin::ref<BinaryGraphNode> next;
};

struct BinaryGraph {
	lf::vector<BinaryGraphNode> nodes;
};

struct BinaryDanglingGraph {
	BinaryGraphNode node;
	BinaryGraphNode dangling;
};

struct BinaryRelocatingGraph {
	lf::vector<BinaryGraphNode> nodes;
};

template<>
struct lf::bin::enum_validator<BinaryValidatedEnum> {
	static constexpr bool is_valid(BinaryValidatedEnum value) {
		return value == BinaryValidatedEnum::Known;
	}
};

template<>
struct lf::bin::polymorphic_traits<BinaryPolyBase> {
	using id_type = u08;
	using types = std::tuple<BinaryPolyA, BinaryPolyB>;

	static id_type id_of(const BinaryPolyBase& value) {
		return value.type_id();
	}

	template<typename Derived>
	static constexpr id_type id_for() {
		if constexpr (std::same_as<Derived, BinaryPolyA>) {
			return 1;
		} else if constexpr (std::same_as<Derived, BinaryPolyB>) {
			return 2;
		}
	}
};

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryPolyA> Value>
lf::error process(Stream& stream, Value& value) {
	return stream(lf::bin::field("value", value.value));
}

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryPolyB> Value>
lf::error process(Stream& stream, Value& value) {
	return stream(lf::bin::field("value", value.value));
}

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryPolyHolder> Value>
lf::error process(Stream& stream, Value& value) {
	return stream(lf::bin::field("item", value.item));
}

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryGraphNode> Value>
lf::error process(Stream& stream, Value& value) {
	return stream(
		lf::bin::field("value", value.value),
		lf::bin::field("next", value.next)
	);
}

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryGraph> Value>
lf::error process(Stream& stream, Value& value) {
	if constexpr (lf::bin::is_writing_stream_v<Stream>) {
		lf::bin::size count = value.nodes.size();
		IF_ERROR_RETURN_ERROR(stream(lf::bin::field("size", count)));
		for (auto& node : value.nodes) {
			IF_ERROR_RETURN_ERROR(lf::bin::process_shared(stream, node));
		}
		return {};
	} else {
		lf::bin::size count = 0;
		IF_ERROR_RETURN_ERROR(stream(lf::bin::field("size", count)));
		value.nodes.resize(static_cast<size_t>(count.value));
		for (BinaryGraphNode& node : value.nodes) {
			IF_ERROR_RETURN_ERROR(lf::bin::process_shared(stream, node));
		}
		return {};
	}
}

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryDanglingGraph> Value>
lf::error process(Stream& stream, Value& value) {
	return stream(lf::bin::field("node", value.node));
}

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryRelocatingGraph> Value>
lf::error process(Stream& stream, Value& value) {
	if constexpr (lf::bin::is_writing_stream_v<Stream>) {
		lf::bin::size count = value.nodes.size();
		IF_ERROR_RETURN_ERROR(stream(lf::bin::field("size", count)));
		for (auto& node : value.nodes) {
			IF_ERROR_RETURN_ERROR(lf::bin::process_shared(stream, node));
		}
		return {};
	} else {
		lf::bin::size count = 0;
		IF_ERROR_RETURN_ERROR(stream(lf::bin::field("size", count)));
		value.nodes.clear();
		value.nodes.reserve(static_cast<size_t>(count.value));
		for (size_t index = 0; index < static_cast<size_t>(count.value); ++index) {
			value.nodes.emplace_back();
			IF_ERROR_RETURN_ERROR(lf::bin::process_shared(stream, value.nodes.back()));
		}
		return {};
	}
}

template<size_t Count, lf::bin::byte_stream Stream, size_t... Indices>
lf::error process_binary_bitmask_values(Stream& stream, std::array<i32, Count>& values, std::index_sequence<Indices...>) {
	return lf::bin::bitmask(
		stream,
		lf::bin::maybe("value", values[Indices], [](i32 value) { return value != 0; }, i32{ 0 })...
	);
}

template<size_t Count>
struct BinaryBitmaskValues {
	std::array<i32, Count> values{};
};

template<lf::bin::byte_stream Stream, size_t Count>
lf::error process(Stream& stream, BinaryBitmaskValues<Count>& value) {
	return process_binary_bitmask_values(stream, value.values, std::make_index_sequence<Count>{});
}

template<lf::bin::writable_byte_stream Stream, size_t Count>
lf::error process(Stream& stream, const BinaryBitmaskValues<Count>& value) {
	auto copy = value.values;
	return process_binary_bitmask_values(stream, copy, std::make_index_sequence<Count>{});
}

inline constexpr lf::version binary_version_1 = lf::version(0, 0, 1, 0);
inline constexpr lf::version binary_version_2 = lf::version(0, 0, 2, 0);
inline constexpr lf::version binary_missing_source_1 = lf::version(1, 0, 1, 0);
inline constexpr lf::version binary_missing_source_2 = lf::version(1, 0, 2, 0);
inline constexpr lf::version binary_missing_migration_1 = lf::version(2, 0, 1, 0);
inline constexpr lf::version binary_missing_migration_2 = lf::version(2, 0, 2, 0);

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryVersionedValue> Value>
lf::error process(Stream& stream, lf::bin::version_tag<binary_version_1>, Value& value) {
	return stream(
		lf::bin::field("value", value.value)
	);
}

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryVersionedValue> Value>
lf::error process(Stream& stream, lf::bin::version_tag<binary_version_2>, Value& value) {
	return stream(
		lf::bin::field("value", value.value),
		lf::bin::field("extra", value.extra)
	);
}

template<>
inline constexpr lf::version lf::bin::migration_source<BinaryVersionedValue, binary_version_2> = binary_version_1;

inline lf::error migrate(lf::bin::version_tag<binary_version_2>, BinaryVersionedValue& value) {
	value.extra = 99;
	return {};
}

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryMissingSourceValue> Value>
lf::error process(Stream& stream, lf::bin::version_tag<binary_missing_source_2>, Value& value) {
	return stream(
		lf::bin::field("value", value.value)
	);
}

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryMissingMigrationValue> Value>
lf::error process(Stream& stream, lf::bin::version_tag<binary_missing_migration_1>, Value& value) {
	return stream(
		lf::bin::field("value", value.value)
	);
}

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryMissingMigrationValue> Value>
lf::error process(Stream& stream, lf::bin::version_tag<binary_missing_migration_2>, Value& value) {
	return stream(
		lf::bin::field("value", value.value)
	);
}

template<>
inline constexpr lf::version lf::bin::migration_source<BinaryMissingMigrationValue, binary_missing_migration_2> =
	binary_missing_migration_1;

u08 byte_at(lf::span<const lf::byte> bytes, size_t index) {
	return std::to_integer<u08>(bytes[index]);
}

template<typename T>
void require_round_trip(const T& value) {
	lf::report<lf::vector<lf::byte>> bytes = lf::bin::write(value);
	REQUIRE(bytes);

	lf::report<T> parsed = lf::bin::read<T>(*bytes);
	REQUIRE(parsed);
	REQUIRE(*parsed == value);
}

template<lf::bin::byte_stream Stream, lf::bin::data<BinaryPlayer> PlayerType>
lf::error process(Stream& stream, PlayerType& player) {
	return stream(
		lf::bin::field("position", player.position),
		lf::bin::field("name", player.name),
		lf::bin::field("health", player.health)
	);
}

/// Verifies that binary concepts accept streams and data cvref.
TEST_CASE("binary concepts accept streams and data cvref", "[binary]") {
	static_assert(lf::bin::byte_stream<lf::bin::read_stream>);
	static_assert(lf::bin::byte_stream<lf::bin::fast_read_stream>);
	static_assert(lf::bin::byte_stream<lf::bin::write_stream>);
	static_assert(lf::bin::byte_stream<lf::bin::fast_write_stream>);
	static_assert(lf::bin::byte_stream<lf::bin::fixed_write_stream>);
	static_assert(lf::bin::readable_byte_stream<lf::bin::read_stream>);
	static_assert(!lf::bin::writable_byte_stream<lf::bin::read_stream>);
	static_assert(!lf::bin::readable_byte_stream<lf::bin::write_stream>);
	static_assert(lf::bin::writable_byte_stream<lf::bin::write_stream>);
	static_assert(lf::bin::data<BinaryPlayer, BinaryPlayer>);
	static_assert(lf::bin::data<const BinaryPlayer, BinaryPlayer>);
	static_assert(!lf::bin::data<i32, BinaryPlayer>);
	static_assert(lf::bin::specialization_of<lf::vector<i32>, lf::vector>);
	static_assert(lf::bin::specialization_of<const lf::vector<i32>&, lf::vector>);
	static_assert(!lf::bin::specialization_of<lf::string, lf::vector>);
	static_assert(lf::bin::binary_field<decltype(lf::bin::field("health", std::declval<i32&>()))>);
	static_assert(lf::bin::binary_field<decltype(lf::bin::field("health", std::declval<const i32&>()))>);
	static_assert(std::same_as<decltype(lf::bin::read<i32>(std::declval<lf::span<const lf::byte>>())), lf::report<i32>>);
	static_assert(std::same_as<decltype(lf::bin::read<i32>(std::declval<lf::span<const lf::byte>>(), lf::bin::read_options{})), lf::report<i32>>);
	static_assert(std::same_as<decltype(lf::bin::fast_read<i32>(std::declval<lf::span<const lf::byte>>())), lf::report<i32>>);
	static_assert(std::same_as<decltype(lf::bin::write(i32{})), lf::report<lf::vector<lf::byte>>>);
	static_assert(std::same_as<decltype(lf::bin::fast_write(i32{})), lf::report<lf::vector<lf::byte>>>);
	static_assert(std::same_as<decltype(lf::bin::measure(i32{})), lf::report<size_t>>);
	static_assert(std::same_as<decltype(lf::bin::write_to(std::declval<lf::span<lf::byte>>(), i32{})), lf::error>);
}

/// Verifies that binary versioned processing dispatches exact layouts and migrations.
TEST_CASE("binary versioned processing dispatches exact layouts and migrations", "[binary]") {
	BinaryVersionedValue exact_v1;
	exact_v1.value = 7;
	exact_v1.extra = 12;

	lf::bin::write_stream exact_v1_out;
	REQUIRE(!lf::bin::process<BinaryVersionedValue, binary_version_1>(exact_v1_out, exact_v1));
	REQUIRE(exact_v1_out.written().size() == sizeof(i32));

	lf::bin::read_stream exact_v1_in(exact_v1_out.written());
	BinaryVersionedValue exact_v1_parsed;
	REQUIRE(!lf::bin::process<BinaryVersionedValue, binary_version_1>(exact_v1_in, exact_v1_parsed));
	REQUIRE(exact_v1_parsed.value == 7);
	REQUIRE(exact_v1_parsed.extra == 0);

	BinaryVersionedValue exact_v2;
	exact_v2.value = 8;
	exact_v2.extra = 13;

	lf::bin::write_stream exact_v2_out;
	REQUIRE(!lf::bin::process<BinaryVersionedValue, binary_version_2>(exact_v2_out, exact_v2));

	lf::bin::read_stream exact_v2_in(exact_v2_out.written());
	BinaryVersionedValue exact_v2_parsed;
	REQUIRE(!lf::bin::process<BinaryVersionedValue, binary_version_2>(exact_v2_in, binary_version_2, exact_v2_parsed));
	REQUIRE(exact_v2_parsed.value == 8);
	REQUIRE(exact_v2_parsed.extra == 13);

	lf::bin::read_stream migrated_in(exact_v1_out.written());
	BinaryVersionedValue migrated;
	REQUIRE(!lf::bin::process<BinaryVersionedValue, binary_version_2>(migrated_in, binary_version_1, migrated));
	REQUIRE(migrated.value == 7);
	REQUIRE(migrated.extra == 99);
}

/// Verifies that binary versioned processing reports missing migration pieces.
TEST_CASE("binary versioned processing reports missing migration pieces", "[binary]") {
	lf::vector<lf::byte> empty;
	lf::bin::read_stream missing_source_stream(empty);
	BinaryMissingSourceValue missing_source;
	lf::error missing_source_error = lf::bin::process<BinaryMissingSourceValue, binary_missing_source_2>(
		missing_source_stream,
		binary_missing_source_1,
		missing_source
	);
	REQUIRE(missing_source_error);

	BinaryMissingMigrationValue source;
	source.value = 42;
	lf::bin::write_stream source_stream;
	REQUIRE(!lf::bin::process<BinaryMissingMigrationValue, binary_missing_migration_1>(source_stream, source));

	lf::bin::read_stream missing_migration_stream(source_stream.written());
	BinaryMissingMigrationValue missing_migration;
	lf::error missing_migration_error = lf::bin::process<BinaryMissingMigrationValue, binary_missing_migration_2>(
		missing_migration_stream,
		binary_missing_migration_1,
		missing_migration
	);
	REQUIRE(missing_migration_error);
	REQUIRE(missing_migration.value == 42);
}

/// Verifies that binary fixed primitives round trip.
TEST_CASE("binary fixed primitives round trip", "[binary]") {
	require_round_trip(u08{ 42 });
	require_round_trip(u16{ 1234 });
	require_round_trip(u32{ 123456 });
	require_round_trip(u64{ 123456789 });
	require_round_trip(i08{ -12 });
	require_round_trip(i16{ -1234 });
	require_round_trip(i32{ -123456 });
	require_round_trip(i64{ -123456789 });
	require_round_trip(f32{ 12.5f });
	require_round_trip(f64{ 123.25 });
	require_round_trip(true);

	lf::report<lf::vector<lf::byte>> bytes = lf::bin::write(i32{ 123 });
	REQUIRE(bytes);
	REQUIRE(bytes->size() == sizeof(i32));

	lf::report<lf::vector<lf::byte>> little_endian = lf::bin::write(u32{ 0x01020304u });
	REQUIRE(little_endian);
	REQUIRE(little_endian->size() == 4);
	REQUIRE(byte_at(*little_endian, 0) == 0x04);
	REQUIRE(byte_at(*little_endian, 1) == 0x03);
	REQUIRE(byte_at(*little_endian, 2) == 0x02);
	REQUIRE(byte_at(*little_endian, 3) == 0x01);

	lf::report<lf::vector<lf::byte>> true_bytes = lf::bin::write(true);
	REQUIRE(true_bytes);
	REQUIRE(true_bytes->size() == 1);
	REQUIRE(byte_at(*true_bytes, 0) == 1);

	lf::vector<lf::byte> invalid_bool;
	invalid_bool.push_back(static_cast<lf::byte>(2));
	lf::report<bool> bool_value = lf::bin::read<bool>(invalid_bool);
	REQUIRE(!bool_value);
}

/// Verifies that binary size uses continuation bytes.
TEST_CASE("binary size uses continuation bytes", "[binary]") {
	lf::report<lf::vector<lf::byte>> single_byte = lf::bin::write(lf::bin::size{ 127 });
	REQUIRE(single_byte);
	REQUIRE(single_byte->size() == 1);
	REQUIRE(byte_at(*single_byte, 0) == 0x7f);
	require_round_trip(lf::bin::size{ 127 });

	lf::report<lf::vector<lf::byte>> two_bytes = lf::bin::write(lf::bin::size{ 128 });
	REQUIRE(two_bytes);
	REQUIRE(two_bytes->size() == 2);
	REQUIRE(byte_at(*two_bytes, 0) == 0x80);
	REQUIRE(byte_at(*two_bytes, 1) == 0x01);
	require_round_trip(lf::bin::size{ 128 });

	lf::report<lf::vector<lf::byte>> three_hundred = lf::bin::write(lf::bin::size{ 300 });
	REQUIRE(three_hundred);
	REQUIRE(three_hundred->size() == 2);
	REQUIRE(byte_at(*three_hundred, 0) == 0xac);
	REQUIRE(byte_at(*three_hundred, 1) == 0x02);
	require_round_trip(lf::bin::size{ 300 });

	constexpr size_t payload_bits_per_byte = 7u;
	constexpr size_t bit_count = sizeof(size_t) * 8u;
	constexpr size_t max_encoded_size_bytes = (bit_count + payload_bits_per_byte - 1u) / payload_bits_per_byte;
	constexpr size_t final_payload_bits = bit_count - ((max_encoded_size_bytes - 1u) * payload_bits_per_byte);
	constexpr u08 final_payload_mask = static_cast<u08>((1u << final_payload_bits) - 1u);

	lf::report<lf::vector<lf::byte>> max_value = lf::bin::write(lf::bin::size{ std::numeric_limits<size_t>::max() });
	REQUIRE(max_value);
	REQUIRE(max_value->size() == max_encoded_size_bytes);
	REQUIRE(byte_at(*max_value, max_encoded_size_bytes - 1u) == final_payload_mask);
	require_round_trip(lf::bin::size{ std::numeric_limits<size_t>::max() });
}

/// Verifies that binary size reports truncated and overflowing reads.
TEST_CASE("binary size reports truncated and overflowing reads", "[binary]") {
	constexpr size_t payload_bits_per_byte = 7u;
	constexpr size_t bit_count = sizeof(size_t) * 8u;
	constexpr size_t max_encoded_size_bytes = (bit_count + payload_bits_per_byte - 1u) / payload_bits_per_byte;
	constexpr size_t final_payload_bits = bit_count - ((max_encoded_size_bytes - 1u) * payload_bits_per_byte);

	lf::vector<lf::byte> truncated;
	truncated.push_back(static_cast<lf::byte>(0x80));
	lf::report<lf::bin::size> truncated_value = lf::bin::read<lf::bin::size>(truncated);
	REQUIRE(!truncated_value);

	lf::vector<lf::byte> overflow;
	overflow.resize(max_encoded_size_bytes, static_cast<lf::byte>(0xff));
	lf::report<lf::bin::size> overflow_value = lf::bin::read<lf::bin::size>(overflow);
	REQUIRE(!overflow_value);

	lf::vector<lf::byte> non_zero_padding;
	non_zero_padding.resize(max_encoded_size_bytes, static_cast<lf::byte>(0x80));
	non_zero_padding[max_encoded_size_bytes - 1u] = static_cast<lf::byte>(1u << final_payload_bits);
	lf::report<lf::bin::size> padded_value = lf::bin::read<lf::bin::size>(non_zero_padding);
	REQUIRE(!padded_value);
}

/// Verifies that binary strings and vectors round trip.
TEST_CASE("binary strings and vectors round trip", "[binary]") {
	require_round_trip(lf::string{});
	require_round_trip(lf::string{ "outposts" });
	require_round_trip(lf::optional<i32>{});
	require_round_trip(lf::optional<i32>{ 42 });

	lf::report<lf::vector<lf::byte>> string_bytes = lf::bin::write(lf::string{ "abc" });
	REQUIRE(string_bytes);
	REQUIRE(string_bytes->size() == 4);
	REQUIRE(byte_at(*string_bytes, 0) == 3);

	lf::vector<lf::byte> truncated_string = *string_bytes;
	truncated_string.pop_back();
	lf::bin::read_stream truncated_string_stream(truncated_string);
	lf::string truncated_string_value;
	lf::error truncated_string_error = truncated_string_stream(lf::bin::field("string", truncated_string_value));
	REQUIRE(truncated_string_error);
	REQUIRE(truncated_string_error.message.find("string") != lf::string::npos);

	lf::vector<i32> empty_values;
	require_round_trip(empty_values);

	lf::vector<i32> values;
	values.push_back(1);
	values.push_back(2);
	values.push_back(3);
	values.push_back(300);
	require_round_trip(values);

	lf::unordered_map<lf::string, i32> lookup;
	lookup["ore"] = 7;
	lookup["water"] = 3;
	require_round_trip(lookup);

	lf::bin::read_stream limited_string_stream(*string_bytes, lf::bin::read_limits{ .max_string_bytes = 2 });
	lf::string limited_string;
	lf::error limited_string_error = limited_string_stream(lf::bin::field("string", limited_string));
	REQUIRE(limited_string_error);
	REQUIRE(limited_string_error.message.find("limit") != lf::string::npos);

	lf::report<lf::vector<lf::byte>> values_bytes = lf::bin::write(values);
	REQUIRE(values_bytes);
	lf::bin::read_stream limited_vector_stream(*values_bytes, lf::bin::read_limits{ .max_vector_elements = 3 });
	lf::vector<i32> limited_values;
	lf::error limited_vector_error = limited_vector_stream(lf::bin::field("values", limited_values));
	REQUIRE(limited_vector_error);
	REQUIRE(limited_vector_error.message.find("limit") != lf::string::npos);

	lf::vector<u32> raw_values;
	raw_values.push_back(0x01020304u);
	raw_values.push_back(0x0a0b0c0du);
	lf::report<lf::vector<lf::byte>> raw_bytes = lf::bin::write(raw_values);
	REQUIRE(raw_bytes);
	REQUIRE(raw_bytes->size() == 9);
	REQUIRE(byte_at(*raw_bytes, 0) == 2);
	REQUIRE(byte_at(*raw_bytes, 1) == 0x04);
	REQUIRE(byte_at(*raw_bytes, 2) == 0x03);
	REQUIRE(byte_at(*raw_bytes, 3) == 0x02);
	REQUIRE(byte_at(*raw_bytes, 4) == 0x01);
	REQUIRE(byte_at(*raw_bytes, 5) == 0x0d);
	REQUIRE(byte_at(*raw_bytes, 6) == 0x0c);
	REQUIRE(byte_at(*raw_bytes, 7) == 0x0b);
	REQUIRE(byte_at(*raw_bytes, 8) == 0x0a);
}

/// Verifies that binary enums round trip as their underlying type.
TEST_CASE("binary enums round trip as their underlying type", "[binary]") {
	lf::report<lf::vector<lf::byte>> bytes = lf::bin::write(BinaryEnum::Beta);
	REQUIRE(bytes);
	REQUIRE(bytes->size() == sizeof(u16));
	REQUIRE(byte_at(*bytes, 0) == 0xcd);
	REQUIRE(byte_at(*bytes, 1) == 0xab);

	lf::report<BinaryEnum> parsed = lf::bin::read<BinaryEnum>(*bytes);
	REQUIRE(parsed);
	REQUIRE(*parsed == BinaryEnum::Beta);

	lf::vector<lf::byte> invalid_validated_enum;
	invalid_validated_enum.push_back(static_cast<lf::byte>(2));
	lf::report<BinaryValidatedEnum> invalid = lf::bin::read<BinaryValidatedEnum>(invalid_validated_enum);
	REQUIRE(!invalid);
}

/// Verifies that binary measure computes serialized byte size.
TEST_CASE("binary measure computes serialized byte size", "[binary]") {
	lf::vector<u32> values;
	values.push_back(0x01020304u);
	values.push_back(0x0a0b0c0du);

	lf::report<lf::vector<lf::byte>> bytes = lf::bin::write(values);
	lf::report<size_t> size = lf::bin::measure(values);
	REQUIRE(bytes);
	REQUIRE(size);
	REQUIRE(*size == bytes->size());
}

/// Verifies that binary fast mode and trivial binary data match normal output.
TEST_CASE("binary fast mode and trivial binary data match normal output", "[binary]") {
	lf::vector<BinaryTrivialRecord> records;
	records.push_back(BinaryTrivialRecord{ 1, 2, 3, 4.0f });
	records.push_back(BinaryTrivialRecord{ 5, 6, 7, 8.0f });

	lf::report<lf::vector<lf::byte>> normal = lf::bin::write(records);
	lf::report<lf::vector<lf::byte>> fast = lf::bin::fast_write(records);
	REQUIRE(normal);
	REQUIRE(fast);
	REQUIRE(*normal == *fast);
	REQUIRE(normal->size() == 1 + records.size() * sizeof(BinaryTrivialRecord));

	lf::report<lf::vector<BinaryTrivialRecord>> parsed = lf::bin::fast_read<lf::vector<BinaryTrivialRecord>>(*fast);
	REQUIRE(parsed);
	REQUIRE(parsed->size() == records.size());
	REQUIRE((*parsed)[0].id == 1);
	REQUIRE((*parsed)[1].weight == 8.0f);

	lf::array<BinaryTrivialRecord, 2> record_array{
		BinaryTrivialRecord{ 9, 10, 11, 12.0f },
		BinaryTrivialRecord{ 13, 14, 15, 16.0f }
	};
	lf::report<lf::vector<lf::byte>> array_bytes = lf::bin::write(record_array);
	REQUIRE(array_bytes);
	REQUIRE(array_bytes->size() == record_array.size() * sizeof(BinaryTrivialRecord));

	lf::report<lf::array<BinaryTrivialRecord, 2>> parsed_array = lf::bin::read<lf::array<BinaryTrivialRecord, 2>>(*array_bytes);
	REQUIRE(parsed_array);
	REQUIRE(*parsed_array == record_array);
}

/// Verifies that binary read stream can expose zero-copy byte spans.
TEST_CASE("binary read stream can expose zero-copy byte spans", "[binary]") {
	lf::byte storage[4]{
		static_cast<lf::byte>(1),
		static_cast<lf::byte>(2),
		static_cast<lf::byte>(3),
		static_cast<lf::byte>(4)
	};
	lf::bin::read_stream stream(storage);
	lf::span<const lf::byte> view;
	REQUIRE(!stream.bytes_view(view, 3));
	REQUIRE(view.size() == 3);
	REQUIRE(byte_at(view, 2) == 3);
	REQUIRE(stream.cursor() == 3);
}

/// Verifies that binary vec2 and custom data round trip.
TEST_CASE("binary vec2 and custom data round trip", "[binary]") {
	require_round_trip(lf::vec2<f32>{ 4.0f, -9.5f });

	BinaryPlayer player;
	player.position = lf::vec2<f32>{ 2.0f, 8.0f };
	player.name = "Leaf";
	player.health = 75;

	const BinaryPlayer& const_player = player;
	lf::report<lf::vector<lf::byte>> bytes = lf::bin::write(const_player);
	REQUIRE(bytes);

	lf::report<BinaryPlayer> parsed = lf::bin::read<BinaryPlayer>(*bytes);
	REQUIRE(parsed);
	REQUIRE(*parsed == player);
}

/// Verifies that binary fixed write stream writes into bounded storage.
TEST_CASE("binary fixed write stream writes into bounded storage", "[binary]") {
	lf::byte storage[4]{};
	lf::error err = lf::bin::write_to(lf::span<lf::byte>(storage), u32{ 0x01020304u });
	REQUIRE(!err);
	REQUIRE(byte_at(storage, 0) == 0x04);
	REQUIRE(byte_at(storage, 1) == 0x03);
	REQUIRE(byte_at(storage, 2) == 0x02);
	REQUIRE(byte_at(storage, 3) == 0x01);

	lf::report<u32> parsed = lf::bin::read<u32>(storage);
	REQUIRE(parsed);
	REQUIRE(*parsed == 0x01020304u);

	lf::byte tiny[3]{};
	lf::error too_small = lf::bin::write_to(lf::span<lf::byte>(tiny), u32{ 0x01020304u });
	REQUIRE(too_small);
	REQUIRE(too_small.message.find("fixed output") != lf::string::npos);

	BinaryPlayer player;
	player.position = lf::vec2<f32>{ 1.0f, 2.0f };
	player.name = "Leaf";
	player.health = 75;

	lf::byte contextual_tiny[2]{};
	lf::error contextual_error = lf::bin::write_to(lf::span<lf::byte>(contextual_tiny), player);
	REQUIRE(contextual_error);
	REQUIRE(contextual_error.message.find("position") != lf::string::npos);
	REQUIRE(contextual_error.message.find("x") != lf::string::npos);
}

/// Verifies that binary truncated primitive read fails.
TEST_CASE("binary truncated primitive read fails", "[binary]") {
	lf::vector<lf::byte> bytes;
	bytes.push_back(static_cast<lf::byte>(0x01));
	lf::report<i32> parsed = lf::bin::read<i32>(bytes);
	REQUIRE(!parsed);
}

/// Verifies that binary read rejects trailing bytes.
TEST_CASE("binary read rejects trailing bytes", "[binary]") {
	lf::vector<lf::byte> bytes;
	bytes.push_back(static_cast<lf::byte>(0x2a));
	bytes.push_back(static_cast<lf::byte>(0x00));
	lf::report<u08> parsed = lf::bin::read<u08>(bytes);
	REQUIRE(!parsed);
	REQUIRE(parsed.error().message.find("trailing byte") != lf::string::npos);

	lf::bin::read_stream stream(bytes);
	u08 value = 0;
	REQUIRE(!stream.process(value));
	REQUIRE(value == 0x2a);
	REQUIRE(stream.cursor() == 1);
}

TEST_CASE("binary graph read resolves public refs and forward refs", "[binary]") {
	BinaryGraph graph;
	graph.nodes.resize(2);
	graph.nodes[0].value = 10;
	graph.nodes[1].value = 20;
	graph.nodes[0].next.ptr = &graph.nodes[1];
	graph.nodes[1].next.ptr = nullptr;

	lf::report<lf::vector<lf::byte>> bytes = lf::bin::write_graph(graph);
	REQUIRE(bytes);

	lf::report<BinaryGraph> parsed = lf::bin::read_graph<BinaryGraph>(*bytes);
	REQUIRE(parsed);
	REQUIRE(parsed->nodes.size() == 2);
	REQUIRE(parsed->nodes[0].value == 10);
	REQUIRE(parsed->nodes[1].value == 20);
	REQUIRE(parsed->nodes[0].next.ptr == &parsed->nodes[1]);
	REQUIRE(parsed->nodes[1].next.ptr == nullptr);
}

TEST_CASE("binary graph write rejects dangling refs", "[binary]") {
	BinaryDanglingGraph graph;
	graph.node.value = 1;
	graph.dangling.value = 2;
	graph.node.next.ptr = &graph.dangling;

	lf::report<lf::vector<lf::byte>> bytes = lf::bin::write_graph(graph);
	REQUIRE(!bytes);
}

TEST_CASE("binary graph fixups survive vector relocation", "[binary]") {
	BinaryRelocatingGraph graph;
	graph.nodes.resize(32);
	for (size_t index = 0; index < graph.nodes.size(); ++index) {
		graph.nodes[index].value = static_cast<i32>(index);
	}
	graph.nodes[0].next.ptr = &graph.nodes[31];

	lf::report<lf::vector<lf::byte>> bytes = lf::bin::write_graph(graph);
	REQUIRE(bytes);

	lf::report<BinaryRelocatingGraph> parsed = lf::bin::read_graph<BinaryRelocatingGraph>(*bytes);
	REQUIRE(parsed);
	REQUIRE(parsed->nodes.size() == 32);
	REQUIRE(parsed->nodes[0].next.ptr == &parsed->nodes[31]);
	REQUIRE(parsed->nodes[0].next.ptr->value == 31);
}

TEST_CASE("binary polymorphic unique ptr writes through const containers", "[binary]") {
	BinaryPolyHolder holder;
	holder.item = lf::make_unique<BinaryPolyB>();
	holder.item->value = 77;

	const BinaryPolyHolder& const_holder = holder;
	lf::report<size_t> measured = lf::bin::measure(const_holder);
	lf::report<lf::vector<lf::byte>> bytes = lf::bin::write(const_holder);
	REQUIRE(measured);
	REQUIRE(bytes);
	REQUIRE(*measured == bytes->size());

	lf::report<BinaryPolyHolder> parsed = lf::bin::read<BinaryPolyHolder>(*bytes);
	REQUIRE(parsed);
	REQUIRE(parsed->item);
	REQUIRE(parsed->item->type_id() == 2);
	REQUIRE(parsed->item->value == 77);
}

TEST_CASE("binary bitmask chooses mask widths by bit count", "[binary]") {
	auto require_bitmask_size = []<size_t Count>(size_t expected_mask_bytes) {
		BinaryBitmaskValues<Count> values;
		values.values[Count - 1u] = 7;

		lf::report<lf::vector<lf::byte>> bytes = lf::bin::write(values);
		REQUIRE(bytes);
		REQUIRE(bytes->size() == expected_mask_bytes + sizeof(i32));

		lf::report<BinaryBitmaskValues<Count>> parsed = lf::bin::read<BinaryBitmaskValues<Count>>(*bytes);
		REQUIRE(parsed);
		REQUIRE(parsed->values[Count - 1u] == 7);
		REQUIRE(parsed->values[0] == (Count == 1 ? 7 : 0));
	};

	require_bitmask_size.template operator()<8>(1);
	require_bitmask_size.template operator()<9>(2);
	require_bitmask_size.template operator()<16>(2);
	require_bitmask_size.template operator()<17>(4);
	require_bitmask_size.template operator()<32>(4);
	require_bitmask_size.template operator()<33>(8);
	require_bitmask_size.template operator()<64>(8);
}

TEST_CASE("binary gated field uses caller owned bitfield enum", "[binary]") {
	BinaryModeStream stream;
	i32 value = 42;
	stream.current_mode = BinaryTestMode::Compact;
	REQUIRE(!lf::bin::gated_field(stream, BinaryTestMode::Full, "value", value));
	REQUIRE(stream.inner.written().empty());

	stream.current_mode = BinaryTestMode::Full;
	REQUIRE(!lf::bin::gated_field(stream, BinaryTestMode::Full, "value", value));
	REQUIRE(stream.inner.written().size() == sizeof(i32));
}

TEST_CASE("binary read and write share logical progress observer", "[binary]") {
	lf::vector<i32> values;
	values.push_back(1);
	values.push_back(2);
	values.push_back(3);

	size_t write_done = 0;
	size_t write_total = 0;
	lf::bin::progress_observer write_progress([&](size_t done, size_t total) {
		REQUIRE(done >= write_done);
		REQUIRE(total >= write_total);
		write_done = done;
		write_total = total;
	});

	lf::report<lf::vector<lf::byte>> bytes = lf::bin::write(values, lf::bin::write_options{ .progress = &write_progress });
	REQUIRE(bytes);
	REQUIRE(write_done > 0);
	REQUIRE(write_total >= write_done);

	size_t read_done = 0;
	size_t read_total = 0;
	lf::bin::progress_observer read_progress([&](size_t done, size_t total) {
		REQUIRE(done >= read_done);
		REQUIRE(total >= read_total);
		read_done = done;
		read_total = total;
	});

	lf::report<lf::vector<i32>> parsed = lf::bin::read<lf::vector<i32>>(*bytes, lf::bin::read_options{ .progress = &read_progress });
	REQUIRE(parsed);
	REQUIRE(*parsed == values);
	REQUIRE(read_done > 0);
	REQUIRE(read_total >= read_done);
}
