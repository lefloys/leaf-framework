#include <concepts>
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

bool operator==(const BinaryPlayer& lhs, const BinaryPlayer& rhs) {
	return lhs.position.x == rhs.position.x
		&& lhs.position.y == rhs.position.y
		&& lhs.name == rhs.name
		&& lhs.health == rhs.health;
}

template <typename T>
void require_round_trip(const T& value) {
	lf::report<lf::vector<u08>> bytes = lf::bin::write(value);
	REQUIRE(bytes);

	lf::report<T> parsed = lf::bin::read<T>(*bytes);
	REQUIRE(parsed);
	REQUIRE(*parsed == value);
}

namespace lf::bin {
	template <byte_stream Stream, data<BinaryPlayer> PlayerType>
	error process(Stream& stream, PlayerType& player) {
		return stream(field("position", player.position), field("name", player.name), field("health", player.health));
	}
} // namespace lf::bin

TEST_CASE("binary concepts accept streams and data cvref", "[binary]") {
	static_assert(lf::bin::byte_stream<lf::bin::read_stream>);
	static_assert(lf::bin::byte_stream<lf::bin::write_stream>);
	static_assert(lf::bin::byte_stream<lf::bin::fixed_write_stream>);
	static_assert(lf::bin::data<BinaryPlayer, BinaryPlayer>);
	static_assert(lf::bin::data<const BinaryPlayer, BinaryPlayer>);
	static_assert(!lf::bin::data<i32, BinaryPlayer>);
	static_assert(lf::bin::specialization_of<lf::vector<i32>, lf::vector>);
	static_assert(lf::bin::specialization_of<const lf::vector<i32>&, lf::vector>);
	static_assert(!lf::bin::specialization_of<lf::string, lf::vector>);
	static_assert(lf::bin::binary_field<decltype(lf::bin::field("health", std::declval<i32&>()))>);
	static_assert(lf::bin::binary_field<decltype(lf::bin::field("health", std::declval<const i32&>()))>);
	static_assert(std::same_as<decltype(lf::bin::read<i32>(std::declval<lf::span<const u08>>())), lf::report<i32>>);
	static_assert(std::same_as<decltype(lf::bin::write(i32{})), lf::report<lf::vector<u08>>>);
	static_assert(
		std::same_as<decltype(lf::bin::write(std::declval<lf::span<u08>>(), i32{})), lf::report<lf::span<const u08>>>);
}

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

	lf::report<lf::vector<u08>> bytes = lf::bin::write(i32{ 123 });
	REQUIRE(bytes);
	REQUIRE(bytes->size() == sizeof(i32));

	lf::report<lf::vector<u08>> little_endian = lf::bin::write(u32{ 0x01020304u });
	REQUIRE(little_endian);
	REQUIRE(little_endian->size() == 4);
	REQUIRE((*little_endian)[0] == 0x04);
	REQUIRE((*little_endian)[1] == 0x03);
	REQUIRE((*little_endian)[2] == 0x02);
	REQUIRE((*little_endian)[3] == 0x01);

	lf::report<lf::vector<u08>> true_bytes = lf::bin::write(true);
	REQUIRE(true_bytes);
	REQUIRE(true_bytes->size() == 1);
	REQUIRE((*true_bytes)[0] == 1);

	lf::vector<u08> invalid_bool;
	invalid_bool.push_back(2);
	lf::report<bool> bool_value = lf::bin::read<bool>(invalid_bool);
	REQUIRE(!bool_value);
}

TEST_CASE("binary size_t uses continuation bytes", "[binary]") {
	lf::report<lf::vector<u08>> single_byte = lf::bin::write(size_t{ 127 });
	REQUIRE(single_byte);
	REQUIRE(single_byte->size() == 1);
	REQUIRE((*single_byte)[0] == 0x7f);
	require_round_trip(size_t{ 127 });

	lf::report<lf::vector<u08>> two_bytes = lf::bin::write(size_t{ 128 });
	REQUIRE(two_bytes);
	REQUIRE(two_bytes->size() == 2);
	REQUIRE((*two_bytes)[0] == 0x80);
	REQUIRE((*two_bytes)[1] == 0x01);
	require_round_trip(size_t{ 128 });

	lf::report<lf::vector<u08>> three_hundred = lf::bin::write(size_t{ 300 });
	REQUIRE(three_hundred);
	REQUIRE(three_hundred->size() == 2);
	REQUIRE((*three_hundred)[0] == 0xac);
	REQUIRE((*three_hundred)[1] == 0x02);
	require_round_trip(size_t{ 300 });

	constexpr size_t payload_bits_per_byte = 7u;
	constexpr size_t bit_count = sizeof(size_t) * 8u;
	constexpr size_t max_encoded_size_bytes = (bit_count + payload_bits_per_byte - 1u) / payload_bits_per_byte;
	constexpr size_t final_payload_bits = bit_count - ((max_encoded_size_bytes - 1u) * payload_bits_per_byte);
	constexpr u08 final_payload_mask = static_cast<u08>((1u << final_payload_bits) - 1u);

	lf::report<lf::vector<u08>> max_value = lf::bin::write(std::numeric_limits<size_t>::max());
	REQUIRE(max_value);
	REQUIRE(max_value->size() == max_encoded_size_bytes);
	REQUIRE((*max_value)[max_encoded_size_bytes - 1u] == final_payload_mask);
	require_round_trip(std::numeric_limits<size_t>::max());
}

TEST_CASE("binary size_t reports truncated and overflowing reads", "[binary]") {
	constexpr size_t payload_bits_per_byte = 7u;
	constexpr size_t bit_count = sizeof(size_t) * 8u;
	constexpr size_t max_encoded_size_bytes = (bit_count + payload_bits_per_byte - 1u) / payload_bits_per_byte;
	constexpr size_t final_payload_bits = bit_count - ((max_encoded_size_bytes - 1u) * payload_bits_per_byte);

	lf::vector<u08> truncated;
	truncated.push_back(0x80);
	lf::report<size_t> truncated_value = lf::bin::read<size_t>(truncated);
	REQUIRE(!truncated_value);

	lf::vector<u08> overflow;
	overflow.resize(max_encoded_size_bytes, 0xff);
	lf::report<size_t> overflow_value = lf::bin::read<size_t>(overflow);
	REQUIRE(!overflow_value);

	lf::vector<u08> non_zero_padding;
	non_zero_padding.resize(max_encoded_size_bytes, 0x80);
	non_zero_padding[max_encoded_size_bytes - 1u] = static_cast<u08>(1u << final_payload_bits);
	lf::report<size_t> padded_value = lf::bin::read<size_t>(non_zero_padding);
	REQUIRE(!padded_value);
}

TEST_CASE("binary strings and vectors round trip", "[binary]") {
	require_round_trip(lf::string{});
	require_round_trip(lf::string{ "outposts" });

	lf::report<lf::vector<u08>> string_bytes = lf::bin::write(lf::string{ "abc" });
	REQUIRE(string_bytes);
	REQUIRE(string_bytes->size() == 4);
	REQUIRE((*string_bytes)[0] == 3);

	lf::bin::read_stream limited_string_stream(*string_bytes, 2);
	lf::string limited_string;
	lf::error limited_string_error = limited_string_stream(lf::bin::field("string", limited_string));
	REQUIRE(limited_string_error);
	REQUIRE(limited_string_error.message.find("string") != lf::string::npos);

	lf::vector<i32> empty_values;
	require_round_trip(empty_values);

	lf::vector<i32> values;
	values.push_back(1);
	values.push_back(2);
	values.push_back(3);
	values.push_back(300);
	require_round_trip(values);
}

TEST_CASE("binary vec2 and custom data round trip", "[binary]") {
	require_round_trip(lf::vec2{ 4.0f, -9.5f });

	BinaryPlayer player;
	player.position = lf::vec2{ 2.0f, 8.0f };
	player.name = "Leaf";
	player.health = 75;

	const BinaryPlayer& const_player = player;
	lf::report<lf::vector<u08>> bytes = lf::bin::write(const_player);
	REQUIRE(bytes);

	lf::report<BinaryPlayer> parsed = lf::bin::read<BinaryPlayer>(*bytes);
	REQUIRE(parsed);
	REQUIRE(*parsed == player);
}

TEST_CASE("binary fixed write stream writes into bounded storage", "[binary]") {
	u08 storage[16]{};
	lf::report<lf::span<const u08>> bytes = lf::bin::write(lf::span<u08>(storage), u32{ 0x01020304u });
	REQUIRE(bytes);
	REQUIRE(bytes->size() == 4);
	REQUIRE((*bytes)[0] == 0x04);
	REQUIRE((*bytes)[1] == 0x03);
	REQUIRE((*bytes)[2] == 0x02);
	REQUIRE((*bytes)[3] == 0x01);

	lf::report<u32> parsed = lf::bin::read<u32>(*bytes);
	REQUIRE(parsed);
	REQUIRE(*parsed == 0x01020304u);

	u08 tiny[3]{};
	lf::report<lf::span<const u08>> too_small = lf::bin::write(lf::span<u08>(tiny), u32{ 0x01020304u });
	REQUIRE(!too_small);
	REQUIRE(too_small.error().message.find("fixed output") != lf::string::npos);

	BinaryPlayer player;
	player.position = lf::vec2{ 1.0f, 2.0f };
	player.name = "Leaf";
	player.health = 75;

	u08 contextual_tiny[2]{};
	lf::report<lf::span<const u08>> contextual_error = lf::bin::write(lf::span<u08>(contextual_tiny), player);
	REQUIRE(!contextual_error);
	REQUIRE(contextual_error.error().message.find("position") != lf::string::npos);
	REQUIRE(contextual_error.error().message.find("x") != lf::string::npos);
}

TEST_CASE("binary truncated primitive read fails", "[binary]") {
	lf::vector<u08> bytes;
	bytes.push_back(0x01);
	lf::report<i32> parsed = lf::bin::read<i32>(bytes);
	REQUIRE(!parsed);
}
