#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "leaf/core/binary.hpp"

struct BinaryPlayer {
	lf::vec2 position{};
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

template<>
struct lf::bin::enum_validator<BinaryValidatedEnum> {
	static constexpr bool is_valid(BinaryValidatedEnum value) {
		return value == BinaryValidatedEnum::Known;
	}
};

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
	require_round_trip(lf::vec2{ 4.0f, -9.5f });

	BinaryPlayer player;
	player.position = lf::vec2{ 2.0f, 8.0f };
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
	player.position = lf::vec2{ 1.0f, 2.0f };
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
