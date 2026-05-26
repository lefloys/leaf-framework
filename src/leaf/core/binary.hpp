#pragma once

#include "error.hpp"
#include "span.hpp"
#include "string.hpp"
#include "types.hpp"
#include "vector.hpp"

#include <bit>
#include <concepts>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

#include <leaf/math/vec.hpp>

namespace lf::bin {
	inline constexpr size_t default_max_sequence_size = 1ull << 26u;

	template <typename T>
	struct field_ref {
		string_view name;
		T& value;
	};

	template <typename T>
	field_ref<T> field(string_view name, T& value) {
		return { name, value };
	}

	template <typename T>
	struct is_field_ref : std::false_type {};

	template <typename T>
	struct is_field_ref<field_ref<T>> : std::true_type {};

	template <typename T>
	concept binary_field = is_field_ref<std::remove_cvref_t<T>>::value;

	namespace detail {
		template <typename Stream>
		string context_message(const Stream& stream, string_view message) {
			if constexpr (requires { stream.context; }) {
				if (!stream.context.empty()) {
					return format("{} : {}", stream.context, message);
				}
			}
			return string(message);
		}

		template <typename Stream>
		struct context_scope {
			Stream& stream;
			string previous;

			context_scope(Stream& stream, string_view where)
				: stream(stream), previous(stream.context) {
				if (where.empty()) {
					return;
				}
				if (!stream.context.empty()) {
					stream.context += " : ";
				}
				stream.context.append(where.data(), where.size());
			}

			~context_scope() {
				stream.context = std::move(previous);
			}
		};

		template <typename Stream>
		void add_context_if_missing(Stream& stream, error& err, string_view action) {
			if (!err || stream.context.empty()) {
				return;
			}
			if (!err.message.starts_with(stream.context)) {
				err.add_context(format("{} : {} binary data failed", stream.context, action));
			}
		}

		template <typename Stream, typename T>
		error process_value(Stream& stream, T& value) {
			return process(stream, value);
		}

		template <typename Stream, binary_field Field>
		error process_field(Stream& stream, Field&& item, string_view action) {
			context_scope scope(stream, item.name);
			if (auto err = stream.process(item.value); err) {
				add_context_if_missing(stream, err, action);
				return err;
			}
			return {};
		}

		template <typename Stream, typename T>
		error write_little_endian(Stream& stream, T value) {
			u08 bytes[sizeof(T)]{};
			for (size_t i = 0; i < sizeof(T); ++i) {
				bytes[i] = static_cast<u08>((value >> (i * 8u)) & 0xffu);
			}
			return stream.bytes(bytes, sizeof(bytes));
		}

		template <typename Stream, typename T>
		error read_little_endian(Stream& stream, T& value) {
			u08 bytes[sizeof(T)]{};
			if (auto err = stream.bytes(bytes, sizeof(bytes)); err) {
				return err;
			}

			value = 0;
			for (size_t i = 0; i < sizeof(T); ++i) {
				value |= static_cast<T>(bytes[i]) << (i * 8u);
			}
			return {};
		}

		template <typename Stream>
		size_t max_sequence_size(const Stream& stream) {
			if constexpr (requires { stream.max_sequence_size; }) {
				return stream.max_sequence_size;
			} else {
				return std::numeric_limits<size_t>::max();
			}
		}

		template <typename T>
		constexpr const char* value_type_name() {
			using raw_t = std::remove_cvref_t<T>;
			if constexpr (std::same_as<raw_t, size_t>) {
				return "size_t";
			} else {
				return lf::type_name<raw_t>();
			}
		}
	}

	struct read_stream {
		span<const u08> data;
		size_t cursor = 0;
		size_t max_sequence_size = default_max_sequence_size;
		string context;

		explicit read_stream(span<const u08> bytes, size_t max_sequence_size = default_max_sequence_size)
			: data(bytes), max_sequence_size(max_sequence_size) {}

		error bytes(void* out, size_t size) {
			if (size > data.size() - cursor) {
				return error(
					generic_errc::input_error,
					detail::context_message(
						*this,
						format(
							"reading {} bytes failed at byte {}: {} bytes remain",
							size,
							cursor,
							data.size() - cursor)));
			}

			if (size > 0) {
				std::memcpy(out, data.data() + cursor, size);
			}
			cursor += size;
			return {};
		}

		template <typename T>
		error process(T& value) {
			return detail::process_value(*this, value);
		}

		template <binary_field... Fields>
		error operator()(Fields&&... fields) {
			error err;
			auto process_one = [&](auto&& item) -> bool {
				err = detail::process_field(*this, std::forward<decltype(item)>(item), "reading");
				return !err;
			};
			(process_one(std::forward<Fields>(fields)) && ...);
			return err;
		}
	};

	struct write_stream {
		vector<u08> data;
		string context;

		error bytes(const void* in, size_t size) {
			if (size == 0) {
				return {};
			}

			const u08* first = static_cast<const u08*>(in);
			data.insert(data.end(), first, first + size);
			return {};
		}

		template <typename T>
		error process(const T& value) {
			return detail::process_value(*this, value);
		}

		template <binary_field... Fields>
		error operator()(Fields&&... fields) {
			error err;
			auto process_one = [&](auto&& item) -> bool {
				err = detail::process_field(*this, std::forward<decltype(item)>(item), "writing");
				return !err;
			};
			(process_one(std::forward<Fields>(fields)) && ...);
			return err;
		}
	};

	struct fixed_write_stream {
		span<u08> data;
		size_t cursor = 0;
		string context;

		explicit fixed_write_stream(span<u08> bytes) : data(bytes) {}

		error bytes(const void* in, size_t size) {
			if (size > data.size() - cursor) {
				return error(
					generic_errc::input_error,
					detail::context_message(
						*this,
						format(
							"writing {} bytes failed at byte {}: fixed output has {} bytes remaining",
							size,
							cursor,
							data.size() - cursor)));
			}

			if (size > 0) {
				std::memcpy(data.data() + cursor, in, size);
			}
			cursor += size;
			return {};
		}

		span<const u08> written() const {
			return span<const u08>(data.data(), cursor);
		}

		template <typename T>
		error process(const T& value) {
			return detail::process_value(*this, value);
		}

		template <binary_field... Fields>
		error operator()(Fields&&... fields) {
			error err;
			auto process_one = [&](auto&& item) -> bool {
				err = detail::process_field(*this, std::forward<decltype(item)>(item), "writing");
				return !err;
			};
			(process_one(std::forward<Fields>(fields)) && ...);
			return err;
		}
	};

	template <typename T>
	concept readable_byte_stream =
		requires(T& stream, void* data, size_t size) {
			{ stream.bytes(data, size) } -> std::same_as<lf::error>;
		};

	template <typename T>
	concept writable_byte_stream =
		requires(T& stream, const void* data, size_t size) {
			{ stream.bytes(data, size) } -> std::same_as<lf::error>;
		};

	template <typename T>
	concept byte_stream = readable_byte_stream<T> || writable_byte_stream<T>;

	// Mutable values are read into, const values are written from.
	template <typename T>
	inline constexpr bool is_writing_v = std::is_const_v<std::remove_reference_t<T>>;

	template <typename T, typename Data>
	concept data = std::same_as<std::remove_cvref_t<T>, Data>;

	template <typename T, template <typename...> typename Template>
	struct is_specialization_of : std::false_type {};

	template <template <typename...> typename Template, typename... Args>
	struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

	template <typename T, template <typename...> typename Template>
	concept specialization_of = is_specialization_of<std::remove_cvref_t<T>, Template>::value;

	template <typename T>
	concept fixed_binary_integer =
		!std::same_as<std::remove_cvref_t<T>, size_t> &&
		(std::same_as<std::remove_cvref_t<T>, u08> ||
		 std::same_as<std::remove_cvref_t<T>, u16> ||
		 std::same_as<std::remove_cvref_t<T>, u32> ||
		 std::same_as<std::remove_cvref_t<T>, u64> ||
		 std::same_as<std::remove_cvref_t<T>, i08> ||
		 std::same_as<std::remove_cvref_t<T>, i16> ||
		 std::same_as<std::remove_cvref_t<T>, i32> ||
		 std::same_as<std::remove_cvref_t<T>, i64>);

	template <typename T>
	concept fixed_binary_float =
		std::same_as<std::remove_cvref_t<T>, f32> ||
		std::same_as<std::remove_cvref_t<T>, f64>;

	template <byte_stream Stream, fixed_binary_integer T>
	error process(Stream& stream, T& value) {
		using raw_t = std::remove_cvref_t<T>;
		using unsigned_t = std::make_unsigned_t<raw_t>;

		if constexpr (is_writing_v<T>) {
			const unsigned_t bits = std::bit_cast<unsigned_t>(value);
			return detail::write_little_endian(stream, bits);
		} else {
			unsigned_t bits = 0;
			if (auto err = detail::read_little_endian(stream, bits); err) {
				return err;
			}
			value = std::bit_cast<raw_t>(bits);
			return {};
		}
	}

	template <byte_stream Stream, fixed_binary_float T>
	error process(Stream& stream, T& value) {
		using raw_t = std::remove_cvref_t<T>;
		using unsigned_t = std::conditional_t<sizeof(raw_t) == sizeof(u32), u32, u64>;

		if constexpr (is_writing_v<T>) {
			const unsigned_t bits = std::bit_cast<unsigned_t>(value);
			return detail::write_little_endian(stream, bits);
		} else {
			unsigned_t bits = 0;
			if (auto err = detail::read_little_endian(stream, bits); err) {
				return err;
			}
			value = std::bit_cast<raw_t>(bits);
			return {};
		}
	}

	template <byte_stream Stream, data<bool> Bool>
	error process(Stream& stream, Bool& value) {
		if constexpr (is_writing_v<Bool>) {
			const u08 byte = value ? 1 : 0;
			return process(stream, byte);
		} else {
			u08 byte = 0;
			if (auto err = process(stream, byte); err) {
				return err;
			}
			if (byte > 1) {
				return error(
					generic_errc::input_error,
					detail::context_message(
						stream,
						format("reading {} failed: expected 0 or 1, got {}", detail::value_type_name<Bool>(), byte)));
			}
			value = byte == 1;
			return {};
		}
	}

	template <byte_stream Stream, data<size_t> Size>
	error process(Stream& stream, Size& value) {
		constexpr size_t payload_bits_per_byte = 7u;
		constexpr size_t bit_count = sizeof(size_t) * 8u;
		constexpr size_t max_encoded_size_bytes = (bit_count + payload_bits_per_byte - 1u) / payload_bits_per_byte;
		constexpr size_t final_payload_bits = bit_count - ((max_encoded_size_bytes - 1u) * payload_bits_per_byte);
		constexpr size_t first_overflow_bit = bit_count;
		constexpr size_t last_overflow_bit = (max_encoded_size_bytes * payload_bits_per_byte) - 1u;
		constexpr u08 final_payload_mask = static_cast<u08>((1u << final_payload_bits) - 1u);

		if constexpr (is_writing_v<Size>) {
			size_t remaining = value;
			do {
				u08 byte = static_cast<u08>(remaining & 0x7fu);
				remaining >>= 7u;
				if (remaining != 0) {
					byte |= 0x80u;
				}

				const u08 out = byte;
				if (auto err = process(stream, out); err) {
					return err;
				}
			} while (remaining != 0);

			return {};
		} else {
			u08 bytes[max_encoded_size_bytes]{};
			size_t byte_count = 0;

			while (true) {
				u08 byte = 0;
				if (auto err = process(stream, byte); err) {
					return err;
				}

				bytes[byte_count] = byte;
				++byte_count;

				if ((byte & 0x80u) == 0) {
					break;
				}
				if (byte_count == max_encoded_size_bytes) {
					return error(
						generic_errc::input_error,
						detail::context_message(
							stream,
							format(
								"reading {} failed: encoded size {} is too big, max is {} bytes",
								detail::value_type_name<Size>(),
								byte_count + 1u,
								max_encoded_size_bytes)));
				}
			}

			if (byte_count == max_encoded_size_bytes) {
				const u08 final_payload = static_cast<u08>(bytes[byte_count - 1u] & 0x7fu);
				if ((final_payload & ~final_payload_mask) != 0) {
					return error(
						generic_errc::input_error,
						detail::context_message(
							stream,
							format(
								"reading {} failed: payload bits {}..{} must be 0, final byte only has {} valid payload bit(s)",
								detail::value_type_name<Size>(),
								first_overflow_bit,
								last_overflow_bit,
								final_payload_bits)));
				}
			}

			size_t result = 0;
			for (size_t i = 0; i < byte_count; ++i) {
				const u08 byte = bytes[i];
				const size_t payload = static_cast<size_t>(byte & 0x7fu);
				result |= payload << (i * 7u);
			}
			value = result;
			return {};
		}
	}

	template <byte_stream Stream, data<string> String>
	error process(Stream& stream, String& value) {
		if constexpr (is_writing_v<String>) {
			const size_t size = value.size();
			if (auto err = stream(field("size", size)); err) {
				return err;
			}
			detail::context_scope scope(stream, "data");
			return stream.bytes(value.data(), size);
		} else {
			size_t size = 0;
			if (auto err = stream(field("size", size)); err) {
				return err;
			}
			if (size > detail::max_sequence_size(stream)) {
				return error(
					generic_errc::input_error,
					detail::context_message(
						stream,
						format(
							"reading {} failed: size {} exceeds max sequence size {}",
							detail::value_type_name<String>(),
							size,
							detail::max_sequence_size(stream))));
			}

			value.resize(size);
			detail::context_scope scope(stream, "data");
			return stream.bytes(value.data(), size);
		}
	}

	template <typename T>
	concept vector_data = specialization_of<T, vector>;

	template <byte_stream Stream, vector_data Vector>
	error process(Stream& stream, Vector& value) {
		if constexpr (is_writing_v<Vector>) {
			const size_t size = value.size();
			if (auto err = stream(field("size", size)); err) {
				return err;
			}

			size_t index = 0;
			for (const auto& item : value) {
				const string item_name = format("[{}]", index);
				if (auto err = stream(field(item_name, item)); err) {
					return err;
				}
				++index;
			}
			return {};
		} else {
			size_t size = 0;
			if (auto err = stream(field("size", size)); err) {
				return err;
			}
			if (size > detail::max_sequence_size(stream)) {
				return error(
					generic_errc::input_error,
					detail::context_message(
						stream,
						format(
							"reading {} failed: size {} exceeds max sequence size {}",
							detail::value_type_name<Vector>(),
							size,
							detail::max_sequence_size(stream))));
			}

			value.resize(size);
			for (size_t index = 0; index < value.size(); ++index) {
				const string item_name = format("[{}]", index);
				if (auto err = stream(field(item_name, value[index])); err) {
					return err;
				}
			}
			return {};
		}
	}

	template <byte_stream Stream, data<vec2> Vec2>
	error process(Stream& stream, Vec2& value) {
		return stream(field("x", value.x), field("y", value.y));
	}

	template <typename T>
	report<T> read(span<const u08> bytes) {
		read_stream stream(bytes);
		T value{};
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		return value;
	}

	template <typename T>
	report<T> process(span<const u08> bytes) {
		return read<T>(bytes);
	}

	template <typename T>
	report<vector<u08>> write(const T& value) {
		write_stream stream;
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		return std::move(stream.data);
	}

	template <typename T>
	report<span<const u08>> write(span<u08> out, const T& value) {
		fixed_write_stream stream(out);
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		return stream.written();
	}
} // namespace lf::bin
