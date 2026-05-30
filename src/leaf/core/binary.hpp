#pragma once

#include "array.hpp"
#include "error.hpp"
#include "memory.hpp"
#include "optional.hpp"
#include "span.hpp"
#include "string.hpp"
#include "types.hpp"
#include "unit.hpp"
#include "unordered_map.hpp"
#include "version.hpp"
#include "vector.hpp"

#include <bit>
#include <concepts>
#include <cstring>
#include <functional>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

#include <leaf/math/vec.hpp>
#include <leaf/math/dim.hpp>
#include <leaf/math/pos.hpp>

template<typename...>
lf::error process(...) requires false;

namespace lf::bin {
	using ::process;

	/*
	** Leaf binary format contract:
	** - Fixed-width integers are encoded little-endian.
	** - Floating-point values are encoded by preserving their IEEE-style bit pattern as
	**   little-endian u32/u64 payloads.
	** - bool is encoded as one byte and canonical reads only accept 0 or 1.
	** - size is encoded as an unsigned 7-bit continuation varint with overflow
	**   rejected on read.
	** - strings and vectors are encoded as size followed by contiguous element data
	**   in logical order.
	** - padding bytes are ignored on read and reserved on write; their contents are undefined.
	** - Versioned formats should dispatch through version_tag and migrate forward one
	**   declared source version at a time.
	** - read<T>() expects the whole input span to be consumed. Use read_stream directly
	**   for embedded/chunked payloads with trailing bytes.
	**/
	struct size {
		size_t value = 0;

		constexpr size() = default;
		constexpr size(size_t value) : value(value) {}

		constexpr operator size_t() const {
			return value;
		}

		constexpr bool operator==(const size&) const = default;
	};

	struct read_limits {
		size_t max_string_bytes = std::numeric_limits<size_t>::max();
		size_t max_vector_elements = std::numeric_limits<size_t>::max();
	};

	struct read_options {
		read_limits limits;
	};

	template<typename T, typename... Args>
	struct field_ref {
		string_view name;
		T& value;
		std::tuple<Args&...> args;
	};

	template<typename T, typename... Args>
	field_ref<T, Args...> field(string_view name, T& value, Args&... args) { return { name, value, { args... } }; }

	template<typename T> struct is_field_ref : std::false_type {};
	template<typename T, typename... Args> struct is_field_ref<field_ref<T, Args...>> : std::true_type {};

	template<typename T>
	struct enum_validator {
		static constexpr bool is_valid(T) {
			return true;
		}
	};

	template<typename T>
	constexpr bool trivially_binary_serializable_v = false;

	template<typename T>
	concept binary_field = is_field_ref<std::remove_cvref_t<T>>::value;

	struct read_stream_tag {};
	struct write_stream_tag {};
	struct fast_stream_tag {};

	template<auto Version>
	struct version_tag { static constexpr auto value = Version; };

	template<size_t Amount>
	struct padding {
		static constexpr size_t amount = Amount;
	};

	template<typename T>
	struct is_padding : std::false_type {};

	template<size_t Amount>
	struct is_padding<padding<Amount>> : std::true_type {};

	template<typename T, template<typename...> typename Template>
	struct is_specialization_of : std::false_type {};

	template<template<typename...> typename Template, typename... Args>
	struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

	template<typename T>
	struct is_array : std::false_type {};

	template<typename Element, size_t Count>
	struct is_array<std::array<Element, Count>> : std::true_type {};

	template<typename T>
	concept readable_byte_stream = requires { typename std::remove_cvref_t<T>::stream_tag; } &&
		std::same_as<typename std::remove_cvref_t<T>::stream_tag, read_stream_tag>;

	template<typename T>
	concept writable_byte_stream = requires { typename std::remove_cvref_t<T>::stream_tag; } &&
		std::same_as<typename std::remove_cvref_t<T>::stream_tag, write_stream_tag>;

	template<typename T>
	concept byte_stream = readable_byte_stream<T> || writable_byte_stream<T>;

	template<typename Stream, typename = void>
	struct is_fast_stream : std::false_type {};

	template<typename Stream>
	struct is_fast_stream<Stream, std::void_t<typename std::remove_cvref_t<Stream>::speed_tag>>
		: std::bool_constant<std::same_as<typename std::remove_cvref_t<Stream>::speed_tag, fast_stream_tag>> {};

	template<typename Stream> constexpr bool is_reading_stream_v = readable_byte_stream<Stream>;
	template<typename Stream> constexpr bool is_writing_stream_v = writable_byte_stream<Stream>;
	template<typename Stream> constexpr bool is_fast_stream_v = is_fast_stream<Stream>::value;

	template<typename T, typename Data>
	concept data = std::same_as<std::remove_cvref_t<T>, Data>;

	template<typename T, template<typename...> typename Template>
	concept specialization_of = is_specialization_of<std::remove_cvref_t<T>, Template>::value;

	template<typename T, template<typename...> typename Template>
	concept specialized_data = data<T, std::remove_cvref_t<T>> && is_specialization_of<std::remove_cvref_t<T>, Template>::value;

	template<typename T>
	concept fixed_binary_integer =
		(std::same_as<std::remove_cvref_t<T>, u08> ||
		 std::same_as<std::remove_cvref_t<T>, u16> ||
		 std::same_as<std::remove_cvref_t<T>, u32> ||
		 std::same_as<std::remove_cvref_t<T>, u64> ||
		 std::same_as<std::remove_cvref_t<T>, i08> ||
		 std::same_as<std::remove_cvref_t<T>, i16> ||
		 std::same_as<std::remove_cvref_t<T>, i32> ||
		 std::same_as<std::remove_cvref_t<T>, i64>);

	template<typename T>
	concept fixed_binary_float =
		std::same_as<std::remove_cvref_t<T>, f32> ||
		std::same_as<std::remove_cvref_t<T>, f64>;

	template<typename T>
	concept trivial_binary_data =
		data<T, std::remove_cvref_t<T>> &&
		std::is_trivially_copyable_v<std::remove_cvref_t<T>> &&
		trivially_binary_serializable_v<std::remove_cvref_t<T>> &&
		!fixed_binary_integer<T> &&
		!fixed_binary_float<T> &&
		!std::same_as<std::remove_cvref_t<T>, bool> &&
		!std::is_enum_v<std::remove_cvref_t<T>>;

	namespace detail {
		template<typename Stream>
		string context_message(const Stream& stream, string_view message) {
			if constexpr (requires { stream.context(); }) {
				if (!stream.context().empty()) {
					return format("{} : {}", stream.context(), message);
				}
			}
			return string(message);
		}

		template<typename Stream>
		struct context_scope {
			Stream& stream;
			string previous;

			context_scope(Stream& stream, string_view where)
				: stream(stream), previous(stream.context()) {
				if (where.empty()) {
					return;
				}
				string next = previous;
				if (!next.empty()) {
					next += " : ";
				}
				next.append(where.data(), where.size());
				stream.set_context(std::move(next));
			}

			~context_scope() {
				stream.set_context(std::move(previous));
			}
		};

		template<typename Stream>
		void add_context_if_missing(Stream& stream, error& err, string_view action) {
			if (!err || stream.context().empty()) {
				return;
			}
			if (!err.message.starts_with(stream.context())) {
				err.add_context(format("{} : {} binary data failed", stream.context(), action));
			}
		}

		template<typename Stream, typename T>
		error process_value(Stream& stream, T& value) {
			return process(stream, value);
		}

		template<typename Stream, binary_field Field>
		error process_field_value(Stream& stream, Field&& item) {
			return std::apply(
				[&](auto&... args) -> error {
					if constexpr (sizeof...(args) == 0) {
						return stream.process(item.value);
					} else {
						return process(stream, item.value, args...);
					}
				},
				item.args
			);
		}

		template<typename Stream, binary_field Field>
		error process_field(Stream& stream, Field&& item, string_view action) {
			if constexpr (is_fast_stream_v<Stream>) {
				return process_field_value(stream, std::forward<Field>(item));
			}

			context_scope scope(stream, item.name);
			if (auto err = process_field_value(stream, std::forward<Field>(item)); err) {
				add_context_if_missing(stream, err, action);
				return err;
			}
			return {};
		}

		template<typename Stream, binary_field... Fields>
		error process_fields(Stream& stream, string_view action, Fields&&... fields) {
			error err;
			auto process_one = [&](auto&& item) -> bool {
				err = process_field(stream, std::forward<decltype(item)>(item), action);
				return !err;
			};
			(process_one(std::forward<Fields>(fields)) && ...);
			return err;
		}

		template<typename Stream, typename T>
		error write_little_endian(Stream& stream, T value) {
			using unsigned_t = std::make_unsigned_t<T>;
			unsigned_t bits = static_cast<unsigned_t>(value);
			lf::byte bytes[sizeof(T)]{};
			for (size_t i = 0; i < sizeof(T); ++i) {
				bytes[i] = static_cast<lf::byte>((bits >> (i * 8u)) & 0xffu);
			}
			return stream.bytes(bytes);
		}

		template<typename Stream, typename T>
		error read_little_endian(Stream& stream, T& value) {
			using unsigned_t = std::make_unsigned_t<T>;
			lf::byte bytes[sizeof(T)]{};
			if (auto err = stream.bytes(bytes); err) {
				return err;
			}

			unsigned_t bits = 0;
			for (size_t i = 0; i < sizeof(T); ++i) {
				bits |= static_cast<unsigned_t>(std::to_integer<u08>(bytes[i])) << (i * 8u);
			}
			value = static_cast<T>(bits);
			return {};
		}

		template<typename Stream, typename T>
		error write_scalar_le(Stream& stream, const T& value) {
			using raw_t = std::remove_cvref_t<T>;
			if constexpr (std::endian::native == std::endian::little) {
				if constexpr (requires { stream.write_scalar(value); }) {
					return stream.write_scalar(value);
				} else {
					return stream.bytes(std::as_bytes(span(&value, 1)));
				}
			} else if constexpr (fixed_binary_integer<raw_t>) {
				using unsigned_t = std::make_unsigned_t<raw_t>;
				return write_little_endian(stream, std::bit_cast<unsigned_t>(value));
			} else {
				using unsigned_t = std::conditional_t<sizeof(raw_t) == sizeof(u32), u32, u64>;
				return write_little_endian(stream, std::bit_cast<unsigned_t>(value));
			}
		}

		template<typename Stream, typename T>
		error read_scalar_le(Stream& stream, T& value) {
			using raw_t = std::remove_cvref_t<T>;
			if constexpr (std::endian::native == std::endian::little) {
				if constexpr (requires { stream.read_scalar(value); }) {
					return stream.read_scalar(value);
				} else {
					return stream.bytes(std::as_writable_bytes(span(&value, 1)));
				}
			} else if constexpr (fixed_binary_integer<raw_t>) {
				using unsigned_t = std::make_unsigned_t<raw_t>;
				unsigned_t bits = 0;
				if (auto err = read_little_endian(stream, bits); err) {
					return err;
				}
				value = std::bit_cast<raw_t>(bits);
				return {};
			} else {
				using unsigned_t = std::conditional_t<sizeof(raw_t) == sizeof(u32), u32, u64>;
				unsigned_t bits = 0;
				if (auto err = read_little_endian(stream, bits); err) {
					return err;
				}
				value = std::bit_cast<raw_t>(bits);
				return {};
			}
		}

		template<typename T>
		constexpr const char* value_type_name() {
			using raw_t = std::remove_cvref_t<T>;
			if constexpr (std::same_as<raw_t, size>) { return "lf::bin::size";
			} else { return lf::type_name<raw_t>(); }
		}

	}

	struct read_stream {
		using stream_tag = read_stream_tag;

		explicit read_stream(span<const lf::byte> bytes) : m_input(bytes) {}
		explicit read_stream(span<const lf::byte> bytes, read_limits limits) : m_input(bytes), m_limits(limits) {}

		error bytes(span<lf::byte> out) {
			if (out.size() > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, format("reading {} bytes failed at byte {}: {} bytes remain", out.size(), m_cursor, remaining())));
			}

			if (!out.empty()) { std::memcpy(out.data(), m_input.data() + m_cursor, out.size()); }
			advance(out.size());
			return {};
		}

		error bytes(void* out, size_t size) {
			return bytes(span<lf::byte>(static_cast<lf::byte*>(out), size));
		}

		template<typename T>
		error read_scalar(T& value) {
			if (sizeof(T) > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, format("reading {} bytes failed at byte {}: {} bytes remain", sizeof(T), m_cursor, remaining())));
			}
			std::memcpy(&value, m_input.data() + m_cursor, sizeof(T));
			advance(sizeof(T));
			return {};
		}

		error bytes_view(span<const lf::byte>& out, size_t byte_count) {
			if (byte_count > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, format("reading {} bytes failed at byte {}: {} bytes remain", byte_count, m_cursor, remaining())));
			}
			out = span<const lf::byte>(m_input.data() + m_cursor, byte_count);
			advance(byte_count);
			return {};
		}

		error padding(size_t size) {
			if (size > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, format("reading {} padding bytes failed at byte {}: {} bytes remain", size, m_cursor, remaining())));
			}
			advance(size);
			return {};
		}

		size_t cursor() const {
			return m_cursor;
		}

		size_t remaining() const {
			return m_input.size() - m_cursor;
		}

		size_t size() const {
			return m_input.size();
		}

		const read_limits& limits() const {
			return m_limits;
		}

		const string& context() const {
			return m_context;
		}

		void set_context(string context) {
			m_context = std::move(context);
		}

		void set_progress(std::function<void(size_t, size_t)> progress) {
			m_progress = std::move(progress);
		}

		template<typename T>
			requires (!std::is_const_v<std::remove_reference_t<T>>)
		error process(T& value) {
			return detail::process_value(*this, value);
		}

		template<binary_field... Fields>
		error operator()(Fields&&... fields) {
			return detail::process_fields(*this, "reading", std::forward<Fields>(fields)...);
		}

	private:
		span<const lf::byte> m_input;
		read_limits m_limits;
		size_t m_cursor = 0;
		string m_context;
		std::function<void(size_t, size_t)> m_progress;

		void advance(size_t amount) {
			m_cursor += amount;
			if (m_progress) {
				m_progress(m_cursor, m_input.size());
			}
		}
	};

	struct fast_read_stream {
		using stream_tag = read_stream_tag;
		using speed_tag = fast_stream_tag;

		explicit fast_read_stream(span<const lf::byte> bytes) : m_input(bytes) {}
		explicit fast_read_stream(span<const lf::byte> bytes, read_limits limits) : m_input(bytes), m_limits(limits) {}

		error bytes(span<lf::byte> out) {
			if (out.size() > remaining()) {
				return error(generic_errc::parse_error, format("reading {} bytes failed at byte {}: {} bytes remain", out.size(), m_cursor, remaining()));
			}
			if (!out.empty()) { std::memcpy(out.data(), m_input.data() + m_cursor, out.size()); }
			m_cursor += out.size();
			return {};
		}

		error bytes(void* out, size_t size) {
			return bytes(span<lf::byte>(static_cast<lf::byte*>(out), size));
		}

		template<typename T>
		error read_scalar(T& value) {
			if (sizeof(T) > remaining()) {
				return error(generic_errc::parse_error, format("reading {} bytes failed at byte {}: {} bytes remain", sizeof(T), m_cursor, remaining()));
			}
			std::memcpy(&value, m_input.data() + m_cursor, sizeof(T));
			m_cursor += sizeof(T);
			return {};
		}

		error bytes_view(span<const lf::byte>& out, size_t byte_count) {
			if (byte_count > remaining()) {
				return error(generic_errc::parse_error, format("reading {} bytes failed at byte {}: {} bytes remain", byte_count, m_cursor, remaining()));
			}
			out = span<const lf::byte>(m_input.data() + m_cursor, byte_count);
			m_cursor += byte_count;
			return {};
		}

		error padding(size_t size) {
			if (size > remaining()) {
				return error(generic_errc::parse_error, format("reading {} padding bytes failed at byte {}: {} bytes remain", size, m_cursor, remaining()));
			}
			m_cursor += size;
			return {};
		}

		size_t cursor() const {
			return m_cursor;
		}

		size_t remaining() const {
			return m_input.size() - m_cursor;
		}

		size_t size() const {
			return m_input.size();
		}

		const read_limits& limits() const {
			return m_limits;
		}

		string_view context() const {
			return {};
		}

		void set_context(string) {}

		template<typename T>
			requires (!std::is_const_v<std::remove_reference_t<T>>)
		error process(T& value) {
			return detail::process_value(*this, value);
		}

		template<binary_field... Fields>
		error operator()(Fields&&... fields) {
			return detail::process_fields(*this, "reading", std::forward<Fields>(fields)...);
		}

	private:
		span<const lf::byte> m_input;
		read_limits m_limits;
		size_t m_cursor = 0;
	};

	struct write_stream {
		using stream_tag = write_stream_tag;

		error bytes(span<const lf::byte> in) {
			if (in.empty()) { return {}; }
			m_output.insert(m_output.end(), in.begin(), in.end());
			return {};
		}

		void reserve(size_t byte_count) {
			m_output.reserve(byte_count);
		}

		error bytes(const void* in, size_t size) {
			return bytes(span<const lf::byte>(static_cast<const lf::byte*>(in), size));
		}

		template<typename T>
		error write_scalar(const T& value) {
			const size_t cursor = m_output.size();
			m_output.resize(cursor + sizeof(T));
			std::memcpy(m_output.data() + cursor, &value, sizeof(T));
			return {};
		}

		error padding(size_t size) {
			m_output.resize(m_output.size() + size);
			return {};
		}

		span<const lf::byte> written() const {
			return m_output;
		}

		vector<lf::byte> take_written() {
			return std::move(m_output);
		}

		const string& context() const {
			return m_context;
		}

		void set_context(string context) {
			m_context = std::move(context);
		}

		template<typename T>
			requires (!std::is_const_v<std::remove_reference_t<T>>)
		error process(T& value) {
			return detail::process_value(*this, value);
		}

		template<typename T>
		error process(const T& value) {
			return detail::process_value(*this, value);
		}

		template<binary_field... Fields>
		error operator()(Fields&&... fields) {
			return detail::process_fields(*this, "writing", std::forward<Fields>(fields)...);
		}

	private:
		vector<lf::byte> m_output;
		string m_context;
	};

	struct fast_write_stream {
		using stream_tag = write_stream_tag;
		using speed_tag = fast_stream_tag;

		error bytes(span<const lf::byte> in) {
			if (in.empty()) { return {}; }
			m_output.insert(m_output.end(), in.begin(), in.end());
			return {};
		}

		error bytes(const void* in, size_t size) {
			return bytes(span<const lf::byte>(static_cast<const lf::byte*>(in), size));
		}

		template<typename T>
		error write_scalar(const T& value) {
			const size_t cursor = m_output.size();
			m_output.resize(cursor + sizeof(T));
			std::memcpy(m_output.data() + cursor, &value, sizeof(T));
			return {};
		}

		error padding(size_t size) {
			m_output.resize(m_output.size() + size);
			return {};
		}

		void reserve(size_t byte_count) {
			m_output.reserve(byte_count);
		}

		span<const lf::byte> written() const {
			return m_output;
		}

		vector<lf::byte> take_written() {
			return std::move(m_output);
		}

		string_view context() const {
			return {};
		}

		void set_context(string) {}

		template<typename T>
			requires (!std::is_const_v<std::remove_reference_t<T>>)
		error process(T& value) {
			return detail::process_value(*this, value);
		}

		template<typename T>
		error process(const T& value) {
			return detail::process_value(*this, value);
		}

		template<binary_field... Fields>
		error operator()(Fields&&... fields) {
			return detail::process_fields(*this, "writing", std::forward<Fields>(fields)...);
		}

	private:
		vector<lf::byte> m_output;
	};

	struct fixed_write_stream {
		using stream_tag = write_stream_tag;

		explicit fixed_write_stream(span<lf::byte> bytes) : m_output(bytes) {}

		error bytes(span<const lf::byte> in) {
			if (in.size() > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, format("writing {} bytes failed at byte {}: fixed output has {} bytes remaining", in.size(), m_cursor, remaining())));
			}
			if (!in.empty()) { std::memcpy(m_output.data() + m_cursor, in.data(), in.size()); }
			m_cursor += in.size();
			return {};
		}

		error bytes(const void* in, size_t size) {
			return bytes(span<const lf::byte>(static_cast<const lf::byte*>(in), size));
		}

		template<typename T>
		error write_scalar(const T& value) {
			if (sizeof(T) > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, format("writing {} bytes failed at byte {}: fixed output has {} bytes remaining", sizeof(T), m_cursor, remaining())));
			}
			std::memcpy(m_output.data() + m_cursor, &value, sizeof(T));
			m_cursor += sizeof(T);
			return {};
		}

		error padding(size_t size) {
			if (size > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, format("writing {} padding bytes failed at byte {}: fixed output has {} bytes remaining", size, m_cursor, remaining())));
			}
			m_cursor += size;
			return {};
		}

		span<const lf::byte> written() const {
			return span<const lf::byte>(m_output.data(), m_cursor);
		}

		size_t cursor() const {
			return m_cursor;
		}

		size_t remaining() const {
			return m_output.size() - m_cursor;
		}

		size_t size() const {
			return m_output.size();
		}

		const string& context() const {
			return m_context;
		}

		void set_context(string context) {
			m_context = std::move(context);
		}

		template<typename T>
			requires (!std::is_const_v<std::remove_reference_t<T>>)
		error process(T& value) {
			return detail::process_value(*this, value);
		}

		template<typename T>
		error process(const T& value) {
			return detail::process_value(*this, value);
		}

		template<binary_field... Fields>
		error operator()(Fields&&... fields) {
			return detail::process_fields(*this, "writing", std::forward<Fields>(fields)...);
		}

	private:
		span<lf::byte> m_output;
		size_t m_cursor = 0;
		string m_context;
	};

	struct measure_stream {
		using stream_tag = write_stream_tag;

		error bytes(span<const lf::byte> in) {
			m_size += in.size();
			return {};
		}

		error bytes(const void*, size_t size) {
			m_size += size;
			return {};
		}

		template<typename T>
		error write_scalar(const T&) {
			m_size += sizeof(T);
			return {};
		}

		error padding(size_t size) {
			m_size += size;
			return {};
		}

		size_t size() const {
			return m_size;
		}

		const string& context() const {
			return m_context;
		}

		void set_context(string context) {
			m_context = std::move(context);
		}

		template<typename T>
			requires (!std::is_const_v<std::remove_reference_t<T>>)
		error process(T& value) {
			return detail::process_value(*this, value);
		}

		template<typename T>
		error process(const T& value) {
			return detail::process_value(*this, value);
		}

		template<binary_field... Fields>
		error operator()(Fields&&... fields) {
			return detail::process_fields(*this, "measuring", std::forward<Fields>(fields)...);
		}

	private:
		size_t m_size = 0;
		string m_context;
	};

	namespace detail {
		struct missing_migration_source {};
	}

	template<typename T, auto TargetVersion>
	constexpr auto migration_source = detail::missing_migration_source{};

	namespace detail {
		template<typename T, auto TargetVersion>
		concept has_migration_source = !std::same_as<std::remove_cvref_t<decltype(migration_source<T, TargetVersion>)>, missing_migration_source>;
	}

	template<typename T, auto Version, byte_stream Stream, data<T> Value>
	error process(Stream& stream, Value& value) {
		return process(stream, version_tag<Version>{}, value);
	}

	template<typename T, auto TargetVersion, readable_byte_stream Stream>
	error migrate(Stream& stream, version file_version, T& value) {
		if constexpr (detail::has_migration_source<T, TargetVersion>) {
			constexpr auto SourceVersion = migration_source<T, TargetVersion>;
			if (auto err = process<T, SourceVersion>(stream, file_version, value); err) {
				return err;
			}
			if constexpr (requires {
					migrate(version_tag<TargetVersion>{}, value);
				}) {
				return migrate(version_tag<TargetVersion>{}, value);
			} else {
				return error(generic_errc::parse_error, detail::context_message(stream, "unsupported binary version"));
			}
		} else {
			return error(generic_errc::parse_error, detail::context_message(stream, "unsupported binary version"));
		}
	}

	template<typename T, auto TargetVersion, readable_byte_stream Stream>
	error process(Stream& stream, version file_version, T& value) {
		if (file_version == TargetVersion) {
			return process<T, TargetVersion>(stream, value);
		}

		return migrate<T, TargetVersion>(stream, file_version, value);
	}

	namespace detail {
		template<typename T>
		constexpr bool bulk_binary_element_v =
			fixed_binary_integer<T> ||
			fixed_binary_float<T> ||
			(std::is_trivially_copyable_v<std::remove_cvref_t<T>> &&
			 trivially_binary_serializable_v<std::remove_cvref_t<T>>);

		template<typename Stream, typename Element>
		error process_bulk_elements(Stream& stream, span<Element> values) {
			if (values.empty()) {
				return {};
			}
			if constexpr (std::endian::native == std::endian::little) {
				if constexpr (is_writing_stream_v<Stream>) {
					return stream.bytes(std::as_bytes(values));
				} else {
					return stream.bytes(std::as_writable_bytes(values));
				}
			} else {
				for (size_t index = 0; index < values.size(); ++index) {
					if constexpr (is_writing_stream_v<Stream>) {
						if (auto err = write_scalar_le(stream, values[index]); err) {
							return err;
						}
					} else {
						if (auto err = read_scalar_le(stream, values[index]); err) {
							return err;
						}
					}
				}
				return {};
			}
		}

		template<typename Stream, typename Vector>
		error validate_vector_allocation(Stream& stream, const Vector& value, size_t item_count) {
			using element_t = typename Vector::value_type;
			if (item_count > value.max_size()) {
				return error(generic_errc::parse_error, context_message(stream, format("reading {} failed: vector size {} exceeds max_size {}", value_type_name<Vector>(), item_count, value.max_size())));
			}
			if constexpr (sizeof(element_t) > 0) {
				constexpr size_t max_count_without_overflow = std::numeric_limits<size_t>::max() / sizeof(element_t);
				if (item_count > max_count_without_overflow) {
					return error(generic_errc::parse_error, context_message(stream, format("reading {} failed: vector byte size overflows size_t", value_type_name<Vector>())));
				}
			}
			return {};
		}
	}

	template<byte_stream Stream, fixed_binary_integer T>
	error process(Stream& stream, T& value) {
		if constexpr (is_writing_stream_v<Stream>) {
			return detail::write_scalar_le(stream, value);
		} else {
			return detail::read_scalar_le(stream, value);
		}
	}

	template<byte_stream Stream, fixed_binary_float T>
	error process(Stream& stream, T& value) {
		if constexpr (is_writing_stream_v<Stream>) {
			return detail::write_scalar_le(stream, value);
		} else {
			return detail::read_scalar_le(stream, value);
		}
	}

	template<byte_stream Stream, data<bool> Bool>
	error process(Stream& stream, Bool& value) {
		if constexpr (is_writing_stream_v<Stream>) {
			const u08 byte = value ? 1 : 0;
			return process(stream, byte);
		} else {
			u08 byte = 0;
			if (auto err = process(stream, byte); err) {
				return err;
			}
			if (byte > 1) {
				return error(generic_errc::parse_error, detail::context_message(stream, format("reading {} failed: expected 0 or 1, got {}", detail::value_type_name<Bool>(), byte)));
			}
			value = (byte != 0);
			return {};
		}
	}

	template<byte_stream Stream, trivial_binary_data T>
	error process(Stream& stream, T& value) {
		if constexpr (is_writing_stream_v<Stream>) {
			if constexpr (requires { stream.write_scalar(value); }) {
				return stream.write_scalar(value);
			} else {
				return stream.bytes(std::as_bytes(span(&value, 1)));
			}
		} else {
			if constexpr (requires { stream.read_scalar(value); }) {
				return stream.read_scalar(value);
			} else {
				return stream.bytes(std::as_writable_bytes(span(&value, 1)));
			}
		}
	}

	template<byte_stream Stream, typename Enum>
		requires data<Enum, std::remove_cvref_t<Enum>> && std::is_enum_v<std::remove_cvref_t<Enum>>
	error process(Stream& stream, Enum& value) {
		using raw_t = std::remove_cvref_t<Enum>;
		using underlying_t = std::underlying_type_t<raw_t>;

		if constexpr (is_writing_stream_v<Stream>) {
			const underlying_t encoded = static_cast<underlying_t>(value);
			return process(stream, encoded);
		} else {
			underlying_t encoded = 0;
			if (auto err = process(stream, encoded); err) {
				return err;
			}
			value = static_cast<raw_t>(encoded);
			if (!enum_validator<raw_t>::is_valid(value)) {
				return error(generic_errc::parse_error, detail::context_message(stream, format("reading {} failed: invalid enum value {}", detail::value_type_name<Enum>(), encoded)));
			}
			return {};
		}
	}

	template<byte_stream Stream, data<size> Size>
	error process(Stream& stream, Size& value) {
		constexpr size_t payload_bits_per_byte = 7u;
		constexpr size_t bit_count = sizeof(size_t) * 8u;
		constexpr size_t max_encoded_size_bytes = (bit_count + payload_bits_per_byte - 1u) / payload_bits_per_byte;
		constexpr size_t final_payload_bits = bit_count - ((max_encoded_size_bytes - 1u) * payload_bits_per_byte);
		constexpr size_t first_overflow_bit = bit_count;
		constexpr size_t last_overflow_bit = (max_encoded_size_bytes * payload_bits_per_byte) - 1u;
		constexpr u08 final_payload_mask = static_cast<u08>((1u << final_payload_bits) - 1u);

		if constexpr (is_writing_stream_v<Stream>) {
			size_t remaining = value.value;
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
			lf::byte bytes[max_encoded_size_bytes]{};
			size_t byte_count = 0;

			while (true) {
				u08 byte = 0;
				if (auto err = process(stream, byte); err) {
					return err;
				}

				bytes[byte_count] = static_cast<lf::byte>(byte);
				++byte_count;

				if ((byte & 0x80u) == 0) {
					break;
				}
				if (byte_count == max_encoded_size_bytes) {
					return error(generic_errc::parse_error, detail::context_message(stream, format("reading {} failed: encoded size {} is too big, max is {} bytes", detail::value_type_name<Size>(), byte_count + 1u, max_encoded_size_bytes)));
				}
			}

			if (byte_count == max_encoded_size_bytes) {
				const u08 final_payload = static_cast<u08>(std::to_integer<u08>(bytes[byte_count - 1u]) & 0x7fu);
				if ((final_payload & ~final_payload_mask) != 0) {
					return error(generic_errc::parse_error, detail::context_message(stream, format("reading {} failed: payload bits {}..{} must be 0, final byte only has {} valid payload bit(s)", detail::value_type_name<Size>(), first_overflow_bit, last_overflow_bit, final_payload_bits)));
				}
			}
			size_t result = 0;
			for (size_t i = 0; i < byte_count; ++i) {
				const u08 byte = std::to_integer<u08>(bytes[i]);
				const size_t payload = static_cast<size_t>(byte & 0x7fu);
				result |= payload << (i * 7u);
			}
			value.value = result;
			return {};
		}
	}

	template<byte_stream Stream, data<string> String>
	error process(Stream& stream, String& value) {
		if constexpr (is_writing_stream_v<Stream>) {
			const size byte_count = value.size();
			if (auto err = stream(field("size", byte_count)); err) {
				return err;
			}
			detail::context_scope scope(stream, "data");
			return stream.bytes(value.data(), value.size());
		} else {
			size byte_count = 0;
			if (auto err = stream(field("size", byte_count)); err) {
				return err;
			}
			if (byte_count.value > stream.limits().max_string_bytes) {
				return error(generic_errc::parse_error, detail::context_message(stream, format("reading {} failed: string byte size {} exceeds limit {}", detail::value_type_name<String>(), byte_count.value, stream.limits().max_string_bytes)));
			}

			string temp;
			temp.resize(static_cast<size_t>(byte_count.value));
			detail::context_scope scope(stream, "data");
			if (auto err = stream.bytes(temp.data(), temp.size()); err) {
				return err;
			}
			value = std::move(temp);
			return {};
		}
	}

	template<byte_stream Stream, typename Vector>
		requires specialized_data<Vector, vector>
	error process(Stream& stream, Vector& value) {
		using element_t = typename std::remove_cvref_t<Vector>::value_type;
		if constexpr (is_writing_stream_v<Stream>) {
			const size item_count = value.size();
			if (auto err = stream(field("size", item_count)); err) {
				return err;
			}

			if constexpr (detail::bulk_binary_element_v<element_t>) {
				detail::context_scope scope(stream, "data");
				return detail::process_bulk_elements(stream, span(value.data(), value.size()));
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
			size item_count = 0;
			if (auto err = stream(field("size", item_count)); err) {
				return err;
			}
			if (item_count.value > stream.limits().max_vector_elements) {
				return error(generic_errc::parse_error, detail::context_message(stream, format("reading {} failed: vector size {} exceeds limit {}", detail::value_type_name<Vector>(), item_count.value, stream.limits().max_vector_elements)));
			}

			std::remove_cvref_t<Vector> temp;
			if (auto err = detail::validate_vector_allocation(stream, temp, item_count.value); err) {
				return err;
			}
			if constexpr (detail::bulk_binary_element_v<element_t>) {
				temp.resize(static_cast<size_t>(item_count.value));
				detail::context_scope scope(stream, "data");
				if (auto err = detail::process_bulk_elements(stream, span(temp.data(), temp.size())); err) {
					return err;
				}
				value = std::move(temp);
				return {};
			}

			temp.reserve(static_cast<size_t>(item_count.value));
			for (size_t index = 0; index < static_cast<size_t>(item_count.value); ++index) {
				temp.emplace_back();
				const string item_name = format("[{}]", index);
				if (auto err = stream(field(item_name, temp.back())); err) {
					return err;
				}
			}
			value = std::move(temp);
			return {};
		}
	}

	template<byte_stream Stream, typename T>
	error process(Stream& stream, unique_ptr<T>& value) {
		bool present = static_cast<bool>(value);
		IF_ERROR_RETURN_ERROR(stream(field("present", present)));
		if (!present) {
			if constexpr (readable_byte_stream<Stream>) {
				value = nullptr;
			}
			return {};
		}
		if constexpr (readable_byte_stream<Stream>) {
			value = make_unique<T>();
		}
		return stream(field("value", *value));
	}

	template<byte_stream Stream, typename T, typename... Args>
		requires (sizeof...(Args) > 0)
	error process(Stream& stream, unique_ptr<T>& value, Args&... args) {
		bool present = static_cast<bool>(value);
		IF_ERROR_RETURN_ERROR(stream(field("present", present)));
		if (!present) {
			if constexpr (readable_byte_stream<Stream>) {
				value = nullptr;
			}
			return {};
		}
		if constexpr (readable_byte_stream<Stream>) {
			value = make_unique<T>();
		}
		return stream(field("value", *value, args...));
	}

	template<byte_stream Stream, typename T>
	error process(Stream& stream, const unique_ptr<T>& value) {
		bool present = static_cast<bool>(value);
		IF_ERROR_RETURN_ERROR(stream(field("present", present)));
		if (!present) {
			return {};
		}
		return stream(field("value", *value));
	}

	template<byte_stream Stream, typename T, typename... Args>
		requires (sizeof...(Args) > 0)
	error process(Stream& stream, const unique_ptr<T>& value, Args&... args) {
		bool present = static_cast<bool>(value);
		IF_ERROR_RETURN_ERROR(stream(field("present", present)));
		if (!present) {
			return {};
		}
		return stream(field("value", *value, args...));
	}

	template<byte_stream Stream, typename Optional>
		requires specialized_data<Optional, optional>
	error process(Stream& stream, Optional& value) {
		bool present = value.has_value();
		IF_ERROR_RETURN_ERROR(stream(field("present", present)));
		if (!present) {
			if constexpr (readable_byte_stream<Stream>) {
				value.reset();
			}
			return {};
		}
		if constexpr (readable_byte_stream<Stream>) {
			value.emplace();
		}
		return stream(field("value", *value));
	}

	template<byte_stream Stream, typename Vector, typename... Args>
		requires (specialized_data<Vector, vector> && sizeof...(Args) > 0)
	error process(Stream& stream, Vector& value, Args&... args) {
		using vector_t = std::remove_cvref_t<Vector>;

		if constexpr (is_writing_stream_v<Stream>) {
			const size item_count = value.size();
			if (auto err = stream(field("size", item_count)); err) {
				return err;
			}

			size_t index = 0;
			for (auto& item : value) {
				const string item_name = format("[{}]", index);
				if (auto err = stream(field(item_name, item, args...)); err) {
					return err;
				}
				++index;
			}
			return {};
		} else {
			size item_count = 0;
			if (auto err = stream(field("size", item_count)); err) {
				return err;
			}
			if (item_count.value > stream.limits().max_vector_elements) {
				return error(generic_errc::parse_error, detail::context_message(stream, format("reading {} failed: vector size {} exceeds limit {}", detail::value_type_name<Vector>(), item_count.value, stream.limits().max_vector_elements)));
			}

			vector_t temp;
			if (auto err = detail::validate_vector_allocation(stream, temp, item_count.value); err) {
				return err;
			}
			temp.reserve(static_cast<size_t>(item_count.value));
			for (size_t index = 0; index < static_cast<size_t>(item_count.value); ++index) {
				temp.emplace_back();
				const string item_name = format("[{}]", index);
				if (auto err = stream(field(item_name, temp.back(), args...)); err) {
					return err;
				}
			}
			value = std::move(temp);
			return {};
		}
	}

	template<byte_stream Stream, typename Optional, typename... Args>
		requires (specialized_data<Optional, optional> && sizeof...(Args) > 0)
	error process(Stream& stream, Optional& value, Args&... args) {
		bool present = value.has_value();
		IF_ERROR_RETURN_ERROR(stream(field("present", present)));
		if (!present) {
			if constexpr (readable_byte_stream<Stream>) {
				value.reset();
			}
			return {};
		}
		if constexpr (readable_byte_stream<Stream>) {
			value.emplace();
		}
		return stream(field("value", *value, args...));
	}

	template<byte_stream Stream, typename Map>
		requires specialized_data<Map, unordered_map>
	error process(Stream& stream, Map& value) {
		using map_t = std::remove_cvref_t<Map>;
		using key_t = typename map_t::key_type;
		using mapped_t = typename map_t::mapped_type;

		if constexpr (is_writing_stream_v<Stream>) {
			const size item_count = value.size();
			if (auto err = stream(field("size", item_count)); err) {
				return err;
			}
			size_t index = 0;
			for (const auto& [key, mapped] : value) {
				const string item_name = format("[{}]", index);
				detail::context_scope scope(stream, item_name);
				if (auto err = stream(field("key", key), field("value", mapped)); err) {
					return err;
				}
				++index;
			}
			return {};
		} else {
			size item_count = 0;
			if (auto err = stream(field("size", item_count)); err) {
				return err;
			}
			if (item_count.value > stream.limits().max_vector_elements) {
				return error(generic_errc::parse_error, detail::context_message(stream, format("reading {} failed: map size {} exceeds limit {}", detail::value_type_name<Map>(), item_count.value, stream.limits().max_vector_elements)));
			}
			map_t temp;
			temp.reserve(static_cast<size_t>(item_count.value));
			for (size_t index = 0; index < static_cast<size_t>(item_count.value); ++index) {
				key_t key{};
				mapped_t mapped{};
				const string item_name = format("[{}]", index);
				detail::context_scope scope(stream, item_name);
				if (auto err = stream(field("key", key), field("value", mapped)); err) {
					return err;
				}
				temp.emplace(std::move(key), std::move(mapped));
			}
			value = std::move(temp);
			return {};
		}
	}

	template<byte_stream Stream, typename Array>
		requires (data<Array, std::remove_cvref_t<Array>> && is_array<std::remove_cvref_t<Array>>::value)
	error process(Stream& stream, Array& value) {
		using element_t = typename std::remove_cvref_t<Array>::value_type;
		if constexpr (detail::bulk_binary_element_v<element_t>) {
			detail::context_scope scope(stream, "data");
			return detail::process_bulk_elements(stream, span(value.data(), value.size()));
		}

		size_t index = 0;
		for (auto& item : value) {
			const string item_name = format("[{}]", index);
			if (auto err = stream(field(item_name, item)); err) {
				return err;
			}
			++index;
		}
		return {};
	}

	template<byte_stream Stream, typename Array, typename... Args>
		requires (data<Array, std::remove_cvref_t<Array>> && is_array<std::remove_cvref_t<Array>>::value && sizeof...(Args) > 0)
	error process(Stream& stream, Array& value, Args&... args) {
		size_t index = 0;
		for (auto& item : value) {
			const string item_name = format("[{}]", index);
			if (auto err = stream(field(item_name, item, args...)); err) {
				return err;
			}
			++index;
		}
		return {};
	}

	template<byte_stream Stream, data<vec2> Vec2>
	error process(Stream& stream, Vec2& value) {
		return stream(field("x", value.x), field("y", value.y));
	}

	template<byte_stream Stream, typename Unit>
		requires specialized_data<Unit, unit>
	error process(Stream& stream, Unit& value) {
		return process(stream, value.value);
	}

	template<byte_stream Stream, typename Pos2>
		requires specialized_data<Pos2, pos2>
	error process(Stream& stream, Pos2& value) {
		return stream(field("x", value.x), field("y", value.y));
	}

	template<byte_stream Stream, typename Dim2>
		requires specialized_data<Dim2, dim2>
	error process(Stream& stream, Dim2& value) {
		return stream(field("width", value.width), field("height", value.height));
	}

	template<byte_stream Stream, data<version> Version>
	error process(Stream& stream, Version& version) {
		return stream(
			field("major", version.major),
			field("minor", version.minor),
			field("patch", version.patch),
			field("snapshot", version.snapshot)
		);
	}

	template<byte_stream Stream, typename Padding>
		requires data<Padding, std::remove_cvref_t<Padding>> && is_padding<std::remove_cvref_t<Padding>>::value
	error process(Stream& stream, Padding&) {
		return stream.padding(std::remove_cvref_t<Padding>::amount);
	}

	template<typename T>
	report<T> read(span<const lf::byte> bytes, read_options options = {}) {
		T value{};
		read_stream stream(bytes, options.limits);
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		if (stream.cursor() != bytes.size()) {
			return unexpected(error(generic_errc::parse_error, detail::context_message(stream, format("reading {} failed: {} trailing byte(s) remain", detail::value_type_name<T>(), bytes.size() - stream.cursor()))));
		}
		return value;
	}

	template<typename T>
	report<T> fast_read(span<const lf::byte> bytes, read_options options = {}) {
		T value{};
		fast_read_stream stream(bytes, options.limits);
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		if (stream.cursor() != bytes.size()) {
			return unexpected(error(generic_errc::parse_error, format("reading {} failed: {} trailing byte(s) remain", detail::value_type_name<T>(), bytes.size() - stream.cursor())));
		}
		return value;
	}

	template<typename T>
	report<T> process(span<const lf::byte> bytes, read_options options = {}) {
		return read<T>(bytes, options);
	}

	template<typename T>
	report<size_t> measure(const T& value) {
		measure_stream stream;
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		return stream.size();
	}

	template<typename T>
	report<vector<lf::byte>> write(const T& value) {
		write_stream stream;
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		return stream.take_written();
	}

	template<typename T>
	report<vector<lf::byte>> fast_write(const T& value) {
		fast_write_stream stream;
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		return stream.take_written();
	}

	template<typename T>
	error write_to(span<lf::byte> out, const T& value) {
		fixed_write_stream stream(out);
		return process(stream, value);
	}
} // namespace lf::bin
