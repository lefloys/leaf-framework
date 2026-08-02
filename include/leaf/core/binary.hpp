#pragma once
#include "leaf/core/schema.hpp"

#include "leaf/core/array.hpp"
#include "leaf/core/concepts.hpp"
#include "leaf/core/error.hpp"
#include "leaf/core/identifier.hpp"
#include "leaf/core/memory.hpp"
#include "leaf/core/optional.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include "leaf/core/unit.hpp"
#include "leaf/core/unordered_map.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/core/version.hpp"

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstring>
#include <functional>
#include <limits>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include <leaf/math/dim.hpp>
#include <leaf/math/pos.hpp>
#include <leaf/math/vec.hpp>

template<typename...>
lf::error process(...)
	requires false;

namespace lf::bin {
	using ::process;

	template<typename Stream, typename T, typename... Args>
	error process(Stream& stream, T& value, Args&... args);

	/*!
	** @ingroup binary
	** @brief Binary serialization and parsing helpers.
	**
	** The binary API is built around one customization point:
	**
	** @code{.cpp}
	** struct player_save {
	**     lf::vec2 position;
	**     lf::string name;
	**     i32 health = 0;
	** };
	**
	** template<lf::bin::byte_stream Stream, lf::bin::data<player_save> Player>
	** lf::error process(Stream& stream, Player& player) {
	**     return stream(
	**         lf::bin::field("position", player.position),
	**         lf::bin::field("name", player.name),
	**         lf::bin::field("health", player.health)
	**     );
	** }
	**
	** lf::report<lf::vector<lf::byte>> bytes = lf::bin::write(player);
	** lf::report<player_save> parsed = lf::bin::read<player_save>(*bytes);
	** @endcode
	**
	** Field names are not written to the byte stream. They are used to build
	** readable diagnostics such as "player : inventory : [3] : id" when parsing
	** or writing fails. For payloads embedded inside a larger packet, construct
	** read_stream or write_stream directly instead of using read<T>().
	**
	** Binary format contract:
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

	/*!
	** @ingroup binary
	** @brief Variable-length encoded size value.
	**
	** size is encoded as an unsigned 7-bit continuation varint. It is used by
	** strings, vectors, maps, and any user format that needs a compact element
	** count or byte count.
	*/
	struct size {
		size_t value = 0;

		constexpr size() = default;
		constexpr size(size_t value) : value(value) {}

		constexpr operator size_t() const {
			return value;
		}

		constexpr bool operator==(const size&) const = default;
	};

	/*!
	** @ingroup binary
	** @brief Allocation and payload limits applied while reading.
	**
	** Use these limits when parsing untrusted or network-provided payloads to
	** reject oversized strings, vectors, and maps before allocation.
	*/
	struct read_limits {
		/*!
		** @brief Maximum string byte length accepted while reading.
		*/
		size_t max_string_bytes = std::numeric_limits<size_t>::max();

		/*!
		** @brief Maximum vector or map element count accepted while reading.
		*/
		size_t max_vector_elements = std::numeric_limits<size_t>::max();
	};

	/*!
	** @ingroup binary
	** @brief Logical progress observer shared by binary readers and writers.
	**
	** Progress is expressed in work units discovered during processing, not
	** exact bytes. Totals may grow while nested containers are processed.
	*/
	struct progress_observer {
		using callback_t = std::function<void(size_t, size_t)>;

		callback_t callback;
		size_t done = 0;
		size_t total = 0;

		progress_observer() = default;
		explicit progress_observer(callback_t callback) : callback(std::move(callback)) {}

		void add_total(size_t amount) {
			total += amount;
			notify();
		}

		void advance(size_t amount = 1) {
			done += amount;
			notify();
		}

		void notify() {
			if (callback) {
				callback(done, total);
			}
		}
	};

	/*!
	** @ingroup binary
	** @brief Options passed to read() and fast_read().
	*/
	struct read_options {
		read_limits limits;
		progress_observer* progress = nullptr;
		// Smooth byte-cursor progress: (cursor, input_size). Fixed total,
		// monotonic — preferred over `progress` for load bars.
		std::function<void(size_t, size_t)> byte_progress;
	};

	/*!
	** @ingroup binary
	** @brief Options passed to write(), fast_write(), write_graph(), and write_to().
	*/
	struct write_options {
		progress_observer* progress = nullptr;
		// Smooth byte-output progress: (bytes_written, byte_progress_total).
		// Set byte_progress_total to measure(value) for an exact denominator.
		std::function<void(size_t, size_t)> byte_progress;
		size_t byte_progress_total = 0;
	};

	/*!
	** @ingroup binary
	** @brief Named field reference passed to a binary stream.
	**
	** field_ref stores the field name, the value to process, and optional extra
	** arguments forwarded to a custom process overload.
	*/
	template<typename T, typename... Args>
	struct field_ref {
		string_view name;
		T& value;
		std::tuple<Args&...> args;
	};

	/*!
	** @ingroup binary
	** @brief Per-hierarchy traits for RTTI-free polymorphic serialization.
	**
	** Specialize this for a base type and provide:
	** - id_type: the encoded discriminator type
	** - types: a tuple of all concrete derived types, in any order
	** - id_of(const Base&): discriminator for a live object
	** - id_for<Derived>(): discriminator for a concrete derived type
	*/
	template<typename Base>
	struct polymorphic_traits;

	template<typename T>
	concept polymorphic = requires {
		typename polymorphic_traits<std::remove_cvref_t<T>>::types;
		typename polymorphic_traits<std::remove_cvref_t<T>>::id_type;
	};

	/*!
	** @ingroup binary
	** @brief Cross-reference context used by graph-aware streams.
	*/
	struct write_refs {
		unordered_map<const void*, u64> ids;
		unordered_map<const void*, u64> definitions;
		u64 next = 0;

		u64 id_of(const void* p) {
			if (!p) {
				return 0;
			}
			auto [it, inserted] = ids.try_emplace(p, next + 1u);
			if (inserted) {
				++next;
			}
			return it->second;
		}

		u64 define(const void* p) {
			const u64 id = id_of(p);
			definitions.emplace(p, id);
			return id;
		}

		error validate() const {
			for (const auto& [ptr, id] : ids) {
				if (ptr && definitions.find(ptr) == definitions.end()) {
					return error(generic_errc::parse_error, "graph ref target was not registered with process_shared");
				}
			}
			return {};
		}
	};

	/*!
	** @ingroup binary
	** @brief Read-side cross-reference table and deferred fixups.
	*/
	struct read_refs {
		struct fixup {
			void* slot = nullptr;
			u64 id = 0;
			void (*assign)(void*, void*) = nullptr;
		};

		unordered_map<u64, void*> objects;
		vector<fixup> fixups;

		void define(u64 id, void* obj) {
			objects.emplace(id, obj);
		}

		template<typename T>
		void defer(T** slot, u64 id) {
			fixups.push_back({ slot,
							   id,
							   [](void* slot, void* object) {
								   *static_cast<T**>(slot) = static_cast<T*>(object);
							   } });
		}

		error run_fixups() {
			for (const fixup& item : fixups) {
				auto it = objects.find(item.id);
				if (it == objects.end()) {
					return error(generic_errc::parse_error, "dangling ref id");
				}
				item.assign(item.slot, it->second);
			}
			fixups.clear();
			return {};
		}
	};

	struct read_stream_tag;
	struct write_stream_tag;
	struct fast_stream_tag;

	template<typename T>
	concept readable_byte_stream = requires { typename std::remove_cvref_t<T>::stream_tag; } &&
								   std::same_as<typename std::remove_cvref_t<T>::stream_tag, read_stream_tag>;

	template<typename T>
	concept writable_byte_stream = requires { typename std::remove_cvref_t<T>::stream_tag; } &&
								   std::same_as<typename std::remove_cvref_t<T>::stream_tag, write_stream_tag>;

	template<typename T>
	concept byte_stream = readable_byte_stream<T> || writable_byte_stream<T>;

	template<typename T, typename... Args>
	struct field_ref;

	template<typename T, typename... Args>
	field_ref<T, Args...> field(string_view name, T& value, Args&... args);

	template<lf::bitfield_enum Mask, byte_stream Stream, typename T>
	error gated_field(Stream& stream, Mask mask, string_view name, T& value) {
		if ((stream.mode() & mask) == Mask{}) {
			return {};
		}
		return stream(field(name, value));
	}

	template<typename T>
	struct ref {
		T* ptr = nullptr;
	};

	template<typename T, typename Pred>
	struct presence {
		string_view name;
		T& value;
		Pred present;
		T fallback;
	};

	template<typename T, typename Pred>
	presence<T, Pred> maybe(string_view name, T& value, Pred present, T fallback = {}) {
		return { name, value, present, fallback };
	}

	template<readable_byte_stream Stream, typename T, typename VNum, typename GNum>
	error process(Stream& stream, ::lf::identifier<T, VNum, GNum>& value) {
		if constexpr (std::is_same_v<GNum, void>) {
			VNum idx = 0;
			IF_ERROR_RETURN_ERROR(stream(field("id", idx)));
			value = ::lf::identifier<T, VNum, GNum>(idx);
			return {};
		} else {
			VNum idx = 0;
			GNum gen = 0;
			IF_ERROR_RETURN_ERROR(stream(field("id", idx), field("gen", gen)));
			value = ::lf::identifier<T, VNum, GNum>(idx, gen);
			return {};
		}
	}

	template<writable_byte_stream Stream, typename T, typename VNum, typename GNum>
	error process(Stream& stream, const ::lf::identifier<T, VNum, GNum>& value) {
		if constexpr (std::is_same_v<GNum, void>) {
			VNum idx = value.get();
			return stream(field("id", idx));
		} else {
			VNum idx = value.get();
			GNum gen = value.gen();
			return stream(field("id", idx), field("gen", gen));
		}
	}

	template<byte_stream Stream, typename... Ps>
	error bitmask(Stream& stream, Ps&&... ps) {
		static_assert(sizeof...(Ps) <= 64, "lf::bin::bitmask supports at most 64 fields");
		using mask_t = std::conditional_t<sizeof...(Ps) <= 8u, u08, std::conditional_t<sizeof...(Ps) <= 16u, u16, std::conditional_t<sizeof...(Ps) <= 32u, u32, u64>>>;
		if constexpr (writable_byte_stream<Stream>) {
			mask_t mask = 0;
			size_t bit = 0;
			((mask |= static_cast<mask_t>(ps.present(ps.value)) << bit++), ...);
			IF_ERROR_RETURN_ERROR(stream(field("mask", mask)));
			size_t index = 0;
			error err;
			auto write_one = [&](auto& p) -> bool {
				if (mask & (static_cast<mask_t>(1) << index++)) {
					err = stream(field(p.name, p.value));
				}
				return !err;
			};
			(write_one(ps) && ...);
			return err;
		} else {
			mask_t mask = 0;
			IF_ERROR_RETURN_ERROR(stream(field("mask", mask)));
			size_t index = 0;
			error err;
			auto read_one = [&](auto& p) -> bool {
				if (mask & (static_cast<mask_t>(1) << index++)) {
					err = stream(field(p.name, p.value));
				} else {
					p.value = p.fallback;
				}
				return !err;
			};
			(read_one(ps) && ...);
			return err;
		}
	}

	namespace detail {
		template<size_t Count>
		struct smallest_uint_for_impl {
			static_assert(Count <= 64, "binary bitmask supports at most 64 fields");
			using type = std::conditional_t<Count <= 8u, u08, std::conditional_t<Count <= 16u, u16, std::conditional_t<Count <= 32u, u32, u64>>>;
		};

		template<size_t Count>
		using smallest_uint_for_t = typename smallest_uint_for_impl<Count>::type;

		template<typename Base, typename Stream, typename Tuple, size_t... Indices>
		error dispatch_polymorphic_write_impl(typename polymorphic_traits<Base>::id_type id, Stream& stream, const Base& value, std::index_sequence<Indices...>) {
			error err = error(generic_errc::parse_error, "unknown polymorphic type id");
			bool matched = ((id == polymorphic_traits<Base>::template id_for<std::tuple_element_t<Indices, Tuple>>() ? (err = process(stream, static_cast<const std::tuple_element_t<Indices, Tuple>&>(value)), true) : false) || ...);
			(void)matched;
			return err;
		}

		template<typename Base, typename Stream, typename Tuple, size_t... Indices>
		error dispatch_polymorphic_read_impl(typename polymorphic_traits<Base>::id_type id, Stream& stream, unique_ptr<Base>& out, std::index_sequence<Indices...>) {
			error err = error(generic_errc::parse_error, "unknown polymorphic type id");
			bool matched = ((id == polymorphic_traits<Base>::template id_for<std::tuple_element_t<Indices, Tuple>>() ? ([&]() -> bool {
								using Derived = std::tuple_element_t<Indices, Tuple>;
								auto value = make_unique<Derived>();
								err = process(stream, *value);
								if (!err) {
									out = std::move(value);
								}
								return true;
							}()) : false) || ...);
			(void)matched;
			return err;
		}
	} // namespace detail

	template<typename Base, typename Stream>
	error dispatch_write(typename polymorphic_traits<Base>::id_type id, Stream& stream, const Base& value) {
		using traits = polymorphic_traits<Base>;
		using tuple_t = typename traits::types;
		return detail::dispatch_polymorphic_write_impl<Base, Stream, tuple_t>(id, stream, value, std::make_index_sequence<std::tuple_size_v<tuple_t>>{});
	}

	template<typename Base, typename Stream>
	error dispatch_read(typename polymorphic_traits<Base>::id_type id, Stream& stream, unique_ptr<Base>& out) {
		using traits = polymorphic_traits<Base>;
		using tuple_t = typename traits::types;
		return detail::dispatch_polymorphic_read_impl<Base, Stream, tuple_t>(id, stream, out, std::make_index_sequence<std::tuple_size_v<tuple_t>>{});
	}

	/*!
	** @ingroup binary
	** @brief Creates a named field reference for stream(...).
	** @param name Diagnostic field name.
	** @param value Value to serialize or parse.
	** @param args Optional arguments forwarded to process(stream, value, args...).
	*/
	template<typename T, typename... Args>
	field_ref<T, Args...> field(string_view name, T& value, Args&... args) { return { name, value, { args... } }; }

	template<typename T>
	struct is_field_ref : std::false_type {};
	template<typename T, typename... Args>
	struct is_field_ref<field_ref<T, Args...>> : std::true_type {};

	template<typename T>
	struct enum_validator {
		static constexpr bool is_valid(T) {
			return true;
		}
	};

	/*!
	** @ingroup binary
	** @brief Opt-in marker for trivially copyable binary payload types.
	**
	** Specialize this variable template to true only for types whose in-memory
	** representation is stable enough to write as raw bytes. Fixed integers,
	** floats, bools, and enums are handled by dedicated portable encoders and do
	** not need this marker.
	*/
	template<typename T>
	constexpr bool trivially_binary_serializable_v = false;

	template<>
	inline constexpr bool trivially_binary_serializable_v<lf::byte> = true;

	template<typename T>
	concept binary_field = is_field_ref<std::remove_cvref_t<T>>::value || schema_node<T>;

	struct read_stream_tag {};
	struct write_stream_tag {};

	struct inherited_context {
		void* m_context = nullptr;
		const std::type_info* m_context_type = nullptr;

		template<typename T>
		void set_context(T& context) {
			m_context = &context;
			m_context_type = &typeid(T);
		}

		template<typename T>
		T& context() const {
			assert(m_context != nullptr && m_context_type != nullptr && *m_context_type == typeid(T));
			return *static_cast<T*>(m_context);
		}
	};

	struct fast_stream_tag {};

	template<auto Version>
	struct version_tag {
		static constexpr auto value = Version;
	};

	/*!
	** @ingroup binary
	** @brief Reserves or skips a fixed number of bytes in a binary layout.
	*/
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

	template<typename Stream, typename = void>
	struct is_fast_stream : std::false_type {};

	template<typename Stream>
	struct is_fast_stream<Stream, std::void_t<typename std::remove_cvref_t<Stream>::speed_tag>>
		: std::bool_constant<std::same_as<typename std::remove_cvref_t<Stream>::speed_tag, fast_stream_tag>> {};

	template<typename Stream>
	constexpr bool is_reading_stream_v = readable_byte_stream<Stream>;
	template<typename Stream>
	constexpr bool is_writing_stream_v = writable_byte_stream<Stream>;
	template<typename Stream>
	constexpr bool is_fast_stream_v = is_fast_stream<Stream>::value;

	template<typename T, typename Data>
	concept data = std::same_as<std::remove_cvref_t<T>, Data>;

	template<typename T, template<typename...> typename Template>
	concept specialization_of = is_specialization_of<std::remove_cvref_t<T>, Template>::value;

	template<typename T, template<typename...> typename Template>
	concept specialized_data = data<T, std::remove_cvref_t<T>> && is_specialization_of<std::remove_cvref_t<T>, Template>::value;

	template<typename T>
	struct is_glm_vec2 : std::false_type {};

	template<typename T, glm::qualifier Qualifier>
	struct is_glm_vec2<glm::vec<2, T, Qualifier>> : std::true_type {};

	template<typename T>
	concept glm_vec2_data = is_glm_vec2<std::remove_cvref_t<T>>::value;

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
					return lf::format("{} : {}", stream.context(), message);
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
				err.add_context(lf::format("{} : {} binary data failed", stream.context(), action));
			}
		}

		template<typename Stream, typename T>
		error process_value(Stream& stream, T& value) {
			return process(stream, value);
		}

		template<typename Stream, typename T, typename Default>
		error process_schema(Stream& stream, const field_node<T, Default>& item) {
			return stream(field_ref<T>{ item.name, item.value, {} });
		}

		template<typename Stream, typename... Fields>
		error process_schema(Stream& stream, const group_node<Fields...>& item) {
			return std::apply([&](const auto&... fields) { return stream(fields...); }, item.fields);
		}

		template<typename Stream, typename Controller, typename Condition, typename... Children>
		error process_schema(Stream& stream, const conditional_node<Controller, Condition, Children...>& item) {
			if (auto err = process_schema(stream, item.controller); err) {
				return err;
			}
			if (!item.condition(item.controller.value)) {
				return {};
			}
			return std::apply([&](const auto&... children) { return stream(children...); }, item.children);
		}

		template<typename Stream, binary_field Field>
		error process_field_value(Stream& stream, Field&& item) {
			if constexpr (schema_node<Field>) {
				return process_schema(stream, item);
			} else {
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
		}

		template<typename Stream, binary_field Field>
		error process_field(Stream& stream, Field&& item, string_view action) {
			if constexpr (is_fast_stream_v<Stream>) {
				if (auto err = process_field_value(stream, std::forward<Field>(item)); err) {
					return err;
				}
				if constexpr (requires { stream.advance_progress(); }) {
					stream.advance_progress();
				}
				return {};
			}

			context_scope scope(stream, item.name);
			if (auto err = process_field_value(stream, std::forward<Field>(item)); err) {
				add_context_if_missing(stream, err, action);
				return err;
			}
			if constexpr (requires { stream.advance_progress(); }) {
				stream.advance_progress();
			}
			return {};
		}

		template<typename Stream, binary_field... Fields>
		error process_fields(Stream& stream, string_view action, Fields&&... fields) {
			if constexpr (requires { stream.add_progress_total(sizeof...(Fields)); }) {
				stream.add_progress_total(sizeof...(Fields));
			}
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
			if constexpr (std::same_as<raw_t, size>) {
				return "lf::bin::size";
			} else {
				return lf::type_name<raw_t>();
			}
		}

	} // namespace detail

	/*!
	** @ingroup binary
	** @brief Checked reader for binary data.
	**
	** read_stream tracks a cursor through an input byte span and preserves
	** field context for diagnostics. Use it directly when a payload is embedded
	** in a larger byte stream or when trailing bytes are expected.
	*/
	struct read_stream : inherited_context {
		using stream_tag = read_stream_tag;
		using inherited_context::context;
		using inherited_context::set_context;

		explicit read_stream(span<const lf::byte> bytes) : m_input(bytes) {}
		explicit read_stream(span<const lf::byte> bytes, read_limits limits) : m_input(bytes), m_limits(limits) {}

		error bytes(span<lf::byte> out) {
			if (out.size() > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, lf::format("reading {} bytes failed at byte {}: {} bytes remain", out.size(), m_cursor, remaining())));
			}

			if (!out.empty()) {
				std::memcpy(out.data(), m_input.data() + m_cursor, out.size());
			}
			advance(out.size());
			return {};
		}

		error bytes(void* out, size_t size) {
			return bytes(span<lf::byte>(static_cast<lf::byte*>(out), size));
		}

		template<typename T>
		error read_scalar(T& value) {
			if (sizeof(T) > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, lf::format("reading {} bytes failed at byte {}: {} bytes remain", sizeof(T), m_cursor, remaining())));
			}
			std::memcpy(&value, m_input.data() + m_cursor, sizeof(T));
			advance(sizeof(T));
			return {};
		}

		/*!
		** @brief Reads a zero-copy view into the remaining input.
		**
		** The returned span points into the original input passed to the stream.
		*/
		error bytes_view(span<const lf::byte>& out, size_t byte_count) {
			if (byte_count > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, lf::format("reading {} bytes failed at byte {}: {} bytes remain", byte_count, m_cursor, remaining())));
			}
			out = span<const lf::byte>(m_input.data() + m_cursor, byte_count);
			advance(byte_count);
			return {};
		}

		error padding(size_t size) {
			if (size > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, lf::format("reading {} padding bytes failed at byte {}: {} bytes remain", size, m_cursor, remaining())));
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

		read_refs& refs() {
			return m_refs;
		}

		const read_refs& refs() const {
			return m_refs;
		}

		const string& context() const {
			return m_context;
		}

		void set_context(string context) {
			m_context = std::move(context);
		}

		/*!
		** @brief Installs a callback invoked whenever the read cursor advances.
		**
		** The callback receives the current cursor and total input size.
		*/
		void set_progress(std::function<void(size_t, size_t)> progress) {
			m_owned_progress = progress_observer(std::move(progress));
			m_progress = &m_owned_progress;
		}

		void set_progress(progress_observer* progress) {
			m_progress = progress;
		}

		/*!
		** @brief Installs a callback reporting raw byte progress.
		**
		** Unlike the logical progress_observer (whose total grows as nested
		** containers are discovered, causing a jumpy ratio), this reports
		** (cursor, input_size) — a fixed total and a monotonically advancing
		** cursor, ideal for a smooth load bar. Calls are throttled so a large
		** read doesn't invoke the callback millions of times.
		*/
		void set_byte_progress(std::function<void(size_t, size_t)> progress) {
			m_byte_progress = std::move(progress);
			m_byte_progress_last = 0;
			const size_t total = m_input.size();
			m_byte_progress_interval = std::clamp<size_t>(total / 400, size_t{ 4096 }, size_t{ 1u << 20 });
			if (m_byte_progress) {
				m_byte_progress(0, total);
			}
		}

		void add_progress_total(size_t amount) {
			if (m_progress) {
				m_progress->add_total(amount);
			}
		}

		void advance_progress(size_t amount = 1) {
			if (m_progress) {
				m_progress->advance(amount);
			}
		}

		template<mutable_type T>
		error process(T& value) {
			return detail::process_value(*this, value);
		}

		/*!
		** @brief Processes named fields in order.
		**
		** This is the usual implementation tool for custom process overloads.
		*/
		template<binary_field... Fields>
		error operator()(Fields&&... fields) {
			return detail::process_fields(*this, "reading", std::forward<Fields>(fields)...);
		}

	  private:
		span<const lf::byte> m_input;
		read_limits m_limits;
		size_t m_cursor = 0;
		read_refs m_refs;
		string m_context;
		progress_observer* m_progress = nullptr;
		progress_observer m_owned_progress;
		std::function<void(size_t, size_t)> m_byte_progress;
		size_t m_byte_progress_last = 0;
		size_t m_byte_progress_interval = 0;

		void advance(size_t amount) {
			m_cursor += amount;
			if (m_byte_progress && m_cursor - m_byte_progress_last >= m_byte_progress_interval) {
				m_byte_progress_last = m_cursor;
				m_byte_progress(m_cursor, m_input.size());
			}
		}
	};

	/*!
	** @ingroup binary
	** @brief Reader variant with reduced diagnostic context overhead.
	**
	** fast_read_stream keeps the same binary format as read_stream but skips
	** field context tracking. Use it for hot paths where detailed parse errors
	** are less important than throughput.
	*/
	struct fast_read_stream : inherited_context {
		using stream_tag = read_stream_tag;
		using inherited_context::context;
		using inherited_context::set_context;
		using speed_tag = fast_stream_tag;

		explicit fast_read_stream(span<const lf::byte> bytes) : m_input(bytes) {}
		explicit fast_read_stream(span<const lf::byte> bytes, read_limits limits) : m_input(bytes), m_limits(limits) {}

		error bytes(span<lf::byte> out) {
			if (out.size() > remaining()) {
				return error(generic_errc::parse_error, lf::format("reading {} bytes failed at byte {}: {} bytes remain", out.size(), m_cursor, remaining()));
			}
			if (!out.empty()) {
				std::memcpy(out.data(), m_input.data() + m_cursor, out.size());
			}
			m_cursor += out.size();
			return {};
		}

		error bytes(void* out, size_t size) {
			return bytes(span<lf::byte>(static_cast<lf::byte*>(out), size));
		}

		template<typename T>
		error read_scalar(T& value) {
			if (sizeof(T) > remaining()) {
				return error(generic_errc::parse_error, lf::format("reading {} bytes failed at byte {}: {} bytes remain", sizeof(T), m_cursor, remaining()));
			}
			std::memcpy(&value, m_input.data() + m_cursor, sizeof(T));
			m_cursor += sizeof(T);
			return {};
		}

		error bytes_view(span<const lf::byte>& out, size_t byte_count) {
			if (byte_count > remaining()) {
				return error(generic_errc::parse_error, lf::format("reading {} bytes failed at byte {}: {} bytes remain", byte_count, m_cursor, remaining()));
			}
			out = span<const lf::byte>(m_input.data() + m_cursor, byte_count);
			m_cursor += byte_count;
			return {};
		}

		error padding(size_t size) {
			if (size > remaining()) {
				return error(generic_errc::parse_error, lf::format("reading {} padding bytes failed at byte {}: {} bytes remain", size, m_cursor, remaining()));
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

		read_refs& refs() {
			return m_refs;
		}

		const read_refs& refs() const {
			return m_refs;
		}

		string_view context() const {
			return {};
		}

		void set_context(string) {}

		void set_progress(progress_observer* progress) {
			m_progress = progress;
		}

		void add_progress_total(size_t amount) {
			if (m_progress) {
				m_progress->add_total(amount);
			}
		}

		void advance_progress(size_t amount = 1) {
			if (m_progress) {
				m_progress->advance(amount);
			}
		}

		template<mutable_type T>
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
		read_refs m_refs;
		progress_observer* m_progress = nullptr;
	};

	/*!
	** @ingroup binary
	** @brief Growing writer for binary data.
	**
	** write_stream appends encoded bytes into an owned vector. Use write() for
	** the common case, or construct write_stream directly when building a larger
	** packet in multiple steps.
	*/
	struct write_stream : inherited_context {
		using stream_tag = write_stream_tag;
		using inherited_context::context;
		using inherited_context::set_context;

		write_stream() = default;

		error bytes(span<const lf::byte> in) {
			if (in.empty()) {
				return {};
			}
			m_output.insert(m_output.end(), in.begin(), in.end());
			notify_bytes();
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
			notify_bytes();
			return {};
		}

		error padding(size_t size) {
			m_output.resize(m_output.size() + size);
			notify_bytes();
			return {};
		}

		/*!
		** @brief Gets the bytes written so far.
		**
		** The returned span remains valid until the stream writes more data or is
		** destroyed.
		*/
		span<const lf::byte> written() const {
			return m_output;
		}

		/*!
		** @brief Moves the written bytes out of the stream.
		*/
		vector<lf::byte> take_written() {
			return std::move(m_output);
		}

		const string& context() const {
			return m_context;
		}

		write_refs& refs() {
			return m_refs;
		}

		const write_refs& refs() const {
			return m_refs;
		}

		void set_context(string context) {
			m_context = std::move(context);
		}

		void set_progress(progress_observer* progress) {
			m_progress = progress;
		}

		/*!
		** @brief Installs a callback reporting raw byte-output progress.
		**
		** Reports (bytes_written, total) where total is the caller-supplied
		** final size (from measure()). Output only grows, so the bar is smooth.
		** Throttled so a large write doesn't call back on every field.
		*/
		void set_byte_progress(std::function<void(size_t, size_t)> progress, size_t total) {
			m_byte_progress = std::move(progress);
			m_byte_progress_total = total;
			m_byte_progress_last = 0;
			m_byte_progress_interval = std::clamp<size_t>(total / 400, size_t{ 4096 }, size_t{ 1u << 20 });
			if (m_byte_progress) {
				m_byte_progress(0, total);
			}
		}

		void add_progress_total(size_t amount) {
			if (m_progress) {
				m_progress->add_total(amount);
			}
		}

		void advance_progress(size_t amount = 1) {
			if (m_progress) {
				m_progress->advance(amount);
			}
		}

		template<mutable_type T>
		error process(T& value) {
			return detail::process_value(*this, value);
		}

		template<typename T>
		error process(const T& value) {
			return detail::process_value(*this, value);
		}

		/*!
		** @brief Processes named fields in order.
		**
		** This is the usual implementation tool for custom process overloads.
		*/
		template<binary_field... Fields>
		error operator()(Fields&&... fields) {
			return detail::process_fields(*this, "writing", std::forward<Fields>(fields)...);
		}

	  private:
		vector<lf::byte> m_output;
		write_refs m_refs;
		progress_observer* m_progress = nullptr;
		string m_context;
		std::function<void(size_t, size_t)> m_byte_progress;
		size_t m_byte_progress_total = 0;
		size_t m_byte_progress_last = 0;
		size_t m_byte_progress_interval = 0;

		void notify_bytes() {
			if (m_byte_progress && m_output.size() - m_byte_progress_last >= m_byte_progress_interval) {
				m_byte_progress_last = m_output.size();
				m_byte_progress(m_output.size(), m_byte_progress_total);
			}
		}
	};

	/*!
	** @ingroup binary
	** @brief Writer variant with reduced diagnostic context overhead.
	**
	** fast_write_stream emits the same bytes as write_stream but skips field
	** context tracking.
	*/
	struct fast_write_stream : inherited_context {
		using stream_tag = write_stream_tag;
		using inherited_context::context;
		using inherited_context::set_context;
		using speed_tag = fast_stream_tag;

		fast_write_stream() = default;

		error bytes(span<const lf::byte> in) {
			if (in.empty()) {
				return {};
			}
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

		write_refs& refs() {
			return m_refs;
		}

		const write_refs& refs() const {
			return m_refs;
		}

		void set_context(string) {}

		void set_progress(progress_observer* progress) {
			m_progress = progress;
		}

		void add_progress_total(size_t amount) {
			if (m_progress) {
				m_progress->add_total(amount);
			}
		}

		void advance_progress(size_t amount = 1) {
			if (m_progress) {
				m_progress->advance(amount);
			}
		}

		template<mutable_type T>
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
		write_refs m_refs;
		progress_observer* m_progress = nullptr;
	};

	/*!
	** @ingroup binary
	** @brief Writer that targets caller-provided fixed storage.
	**
	** fixed_write_stream is useful when a protocol requires writing into an
	** existing buffer. Writes fail with parse_error if the output span is too
	** small.
	*/
	struct fixed_write_stream : inherited_context {
		using stream_tag = write_stream_tag;
		using inherited_context::context;
		using inherited_context::set_context;

		explicit fixed_write_stream(span<lf::byte> bytes) : m_output(bytes) {}

		error bytes(span<const lf::byte> in) {
			if (in.size() > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, lf::format("writing {} bytes failed at byte {}: fixed output has {} bytes remaining", in.size(), m_cursor, remaining())));
			}
			if (!in.empty()) {
				std::memcpy(m_output.data() + m_cursor, in.data(), in.size());
			}
			m_cursor += in.size();
			return {};
		}

		error bytes(const void* in, size_t size) {
			return bytes(span<const lf::byte>(static_cast<const lf::byte*>(in), size));
		}

		template<typename T>
		error write_scalar(const T& value) {
			if (sizeof(T) > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, lf::format("writing {} bytes failed at byte {}: fixed output has {} bytes remaining", sizeof(T), m_cursor, remaining())));
			}
			std::memcpy(m_output.data() + m_cursor, &value, sizeof(T));
			m_cursor += sizeof(T);
			return {};
		}

		error padding(size_t size) {
			if (size > remaining()) {
				return error(generic_errc::parse_error, detail::context_message(*this, lf::format("writing {} padding bytes failed at byte {}: fixed output has {} bytes remaining", size, m_cursor, remaining())));
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

		write_refs& refs() {
			return m_refs;
		}

		const write_refs& refs() const {
			return m_refs;
		}

		void set_progress(progress_observer* progress) {
			m_progress = progress;
		}

		void add_progress_total(size_t amount) {
			if (m_progress) {
				m_progress->add_total(amount);
			}
		}

		void advance_progress(size_t amount = 1) {
			if (m_progress) {
				m_progress->advance(amount);
			}
		}

		template<mutable_type T>
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
		write_refs m_refs;
		progress_observer* m_progress = nullptr;
		string m_context;
	};

	/*!
	** @ingroup binary
	** @brief Writer-like stream that counts bytes without storing them.
	**
	** measure_stream follows the same process path as writing, making it useful
	** for pre-sizing output buffers.
	*/
	struct measure_stream {
		using stream_tag = write_stream_tag;
		using speed_tag = fast_stream_tag;

		measure_stream() = default;

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

		write_refs& refs() {
			return m_refs;
		}

		const write_refs& refs() const {
			return m_refs;
		}

		const string& context() const {
			return m_context;
		}

		void set_context(string context) {
			m_context = std::move(context);
		}

		void set_progress(progress_observer* progress) {
			m_progress = progress;
		}

		void add_progress_total(size_t amount) {
			if (m_progress) {
				m_progress->add_total(amount);
			}
		}

		void advance_progress(size_t amount = 1) {
			if (m_progress) {
				m_progress->advance(amount);
			}
		}

		template<mutable_type T>
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
		write_refs m_refs;
		progress_observer* m_progress = nullptr;
		string m_context;
	};

	namespace detail {
		struct missing_migration_source {};
	} // namespace detail

	template<typename T, auto TargetVersion>
	constexpr auto migration_source = detail::missing_migration_source{};

	namespace detail {
		template<typename T, auto TargetVersion>
		concept has_migration_source = !std::same_as<std::remove_cvref_t<decltype(migration_source<T, TargetVersion>)>, missing_migration_source>;
	}

	/*!
	** @ingroup binary
	** @brief Processes a value with a specific binary schema version.
	**
	** Define process(stream, version_tag<Version>{}, value) overloads for each
	** layout version, then call this helper when writing or when reading bytes
	** already known to match Version exactly.
	*/
	template<typename T, auto Version, byte_stream Stream, data<T> Value>
	error process(Stream& stream, Value& value) {
		return process(stream, version_tag<Version>{}, value);
	}

	/*!
	** @ingroup binary
	** @brief Migrates data from its encoded file version to a target version.
	**
	** Specialize migration_source<T, TargetVersion> with the immediately
	** preceding version, and provide migrate(version_tag<TargetVersion>, value)
	** to transform the already-read value one version step forward.
	*/
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

	/*!
	** @ingroup binary
	** @brief Reads a versioned value and migrates it to TargetVersion if needed.
	*/
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
					IF_ERROR_RETURN_ERROR(stream.bytes(std::as_bytes(values)));
				} else {
					IF_ERROR_RETURN_ERROR(stream.bytes(std::as_writable_bytes(values)));
				}
				if constexpr (requires { stream.advance_progress(values.size()); }) {
					stream.advance_progress(values.size());
				}
				return {};
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
					if constexpr (requires { stream.advance_progress(); }) {
						stream.advance_progress();
					}
				}
				return {};
			}
		}

		template<typename Stream, typename Vector>
		error validate_vector_allocation(Stream& stream, const Vector& value, size_t item_count) {
			using element_t = typename Vector::value_type;
			if (item_count > value.max_size()) {
				return error(generic_errc::parse_error, context_message(stream, lf::format("reading {} failed: vector size {} exceeds max_size {}", value_type_name<Vector>(), item_count, value.max_size())));
			}
			if constexpr (sizeof(element_t) > 0) {
				constexpr size_t max_count_without_overflow = std::numeric_limits<size_t>::max() / sizeof(element_t);
				if (item_count > max_count_without_overflow) {
					return error(generic_errc::parse_error, context_message(stream, lf::format("reading {} failed: vector byte size overflows size_t", value_type_name<Vector>())));
				}
			}
			return {};
		}
	} // namespace detail

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
				return error(generic_errc::parse_error, detail::context_message(stream, lf::format("reading {} failed: expected 0 or 1, got {}", detail::value_type_name<Bool>(), byte)));
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
				return error(generic_errc::parse_error, detail::context_message(stream, lf::format("reading {} failed: invalid enum value {}", detail::value_type_name<Enum>(), encoded)));
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
					return error(generic_errc::parse_error, detail::context_message(stream, lf::format("reading {} failed: encoded size {} is too big, max is {} bytes", detail::value_type_name<Size>(), byte_count + 1u, max_encoded_size_bytes)));
				}
			}

			if (byte_count == max_encoded_size_bytes) {
				const u08 final_payload = static_cast<u08>(std::to_integer<u08>(bytes[byte_count - 1u]) & 0x7fu);
				if ((final_payload & ~final_payload_mask) != 0) {
					return error(generic_errc::parse_error, detail::context_message(stream, lf::format("reading {} failed: payload bits {}..{} must be 0, final byte only has {} valid payload bit(s)", detail::value_type_name<Size>(), first_overflow_bit, last_overflow_bit, final_payload_bits)));
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
				return error(generic_errc::parse_error, detail::context_message(stream, lf::format("reading {} failed: string byte size {} exceeds limit {}", detail::value_type_name<String>(), byte_count.value, stream.limits().max_string_bytes)));
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
			if constexpr (requires { stream.add_progress_total(value.size()); }) {
				stream.add_progress_total(value.size());
			}

			if constexpr (detail::bulk_binary_element_v<element_t>) {
				detail::context_scope scope(stream, "data");
				return detail::process_bulk_elements(stream, span(value.data(), value.size()));
			}

			size_t index = 0;
			for (const auto& item : value) {
				const string item_name = lf::format("[{}]", index);
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
				return error(generic_errc::parse_error, detail::context_message(stream, lf::format("reading {} failed: vector size {} exceeds limit {}", detail::value_type_name<Vector>(), item_count.value, stream.limits().max_vector_elements)));
			}
			if constexpr (requires { stream.add_progress_total(item_count.value); }) {
				stream.add_progress_total(item_count.value);
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
				const string item_name = lf::format("[{}]", index);
				if (auto err = stream(field(item_name, temp.back())); err) {
					return err;
				}
			}
			value = std::move(temp);
			return {};
		}
	}

	template<readable_byte_stream Stream, typename T, typename... Args>
		requires(!polymorphic<T>)
	error process(Stream& stream, unique_ptr<T>& value, Args&... args) {
		bool present = static_cast<bool>(value);
		IF_ERROR_RETURN_ERROR(stream(field("present", present)));
		if (!present) {
			value = nullptr;
			return {};
		}
		value = make_unique<T>();
		return stream(field("value", *value, args...));
	}

	template<writable_byte_stream Stream, typename T, typename... Args>
		requires(!polymorphic<T>)
	error process(Stream& stream, const unique_ptr<T>& value, Args&... args) {
		bool present = static_cast<bool>(value);
		IF_ERROR_RETURN_ERROR(stream(field("present", present)));
		if (present) {
			return stream(field("value", *value, args...));
		}
		return {};
	}

	template<readable_byte_stream Stream, typename Base, typename... Args>
		requires polymorphic<Base>
	error process(Stream& stream, unique_ptr<Base>& value, Args&... args) {
		using traits = polymorphic_traits<Base>;
		using id_t = typename traits::id_type;

		bool present = static_cast<bool>(value);
		IF_ERROR_RETURN_ERROR(stream(field("present", present)));
		if (!present) {
			value = nullptr;
			return {};
		}
		id_t id{};
		IF_ERROR_RETURN_ERROR(stream(field("type", id)));
		return dispatch_read<Base>(id, stream, value);
	}

	template<writable_byte_stream Stream, typename Base, typename... Args>
		requires polymorphic<Base>
	error process(Stream& stream, const unique_ptr<Base>& value, Args&...) {
		using traits = polymorphic_traits<Base>;
		using id_t = typename traits::id_type;

		bool present = static_cast<bool>(value);
		IF_ERROR_RETURN_ERROR(stream(field("present", present)));
		if (!present) {
			return {};
		}
		id_t id = traits::id_of(*value);
		IF_ERROR_RETURN_ERROR(stream(field("type", id)));
		return dispatch_write<Base>(id, stream, *value);
	}

	template<readable_byte_stream Stream, typename T>
		requires requires(Stream& s) { s.refs(); }
	error process(Stream& stream, ref<T>& value) {
		u64 id = 0;
		IF_ERROR_RETURN_ERROR(stream(field("ref", id)));
		if (id == 0) {
			value.ptr = nullptr;
			return {};
		}
		stream.refs().defer(&value.ptr, id);
		return {};
	}

	template<writable_byte_stream Stream, typename T>
		requires requires(Stream& s) { s.refs(); }
	error process(Stream& stream, const ref<T>& value) {
		u64 id = stream.refs().id_of(value.ptr);
		return stream(field("ref", id));
	}

	template<readable_byte_stream Stream, typename T>
		requires requires(Stream& s) { s.refs(); }
	error process_shared(Stream& stream, T& value) {
		if constexpr (requires { stream.add_progress_total(1); }) {
			stream.add_progress_total(1);
		}
		u64 id = 0;
		IF_ERROR_RETURN_ERROR(stream(field("id", id)));
		stream.refs().define(id, &value);
		IF_ERROR_RETURN_ERROR(process(stream, value));
		if constexpr (requires { stream.advance_progress(); }) {
			stream.advance_progress();
		}
		return {};
	}

	template<writable_byte_stream Stream, typename T>
		requires requires(Stream& s) { s.refs(); }
	error process_shared(Stream& stream, const T& value) {
		if constexpr (requires { stream.add_progress_total(1); }) {
			stream.add_progress_total(1);
		}
		u64 id = stream.refs().define(&value);
		IF_ERROR_RETURN_ERROR(stream(field("id", id)));
		IF_ERROR_RETURN_ERROR(process(stream, value));
		if constexpr (requires { stream.advance_progress(); }) {
			stream.advance_progress();
		}
		return {};
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
		requires(specialized_data<Vector, vector> && sizeof...(Args) > 0)
	error process(Stream& stream, Vector& value, Args&... args) {
		using vector_t = std::remove_cvref_t<Vector>;

		if constexpr (is_writing_stream_v<Stream>) {
			const size item_count = value.size();
			if (auto err = stream(field("size", item_count)); err) {
				return err;
			}
			if constexpr (requires { stream.add_progress_total(value.size()); }) {
				stream.add_progress_total(value.size());
			}

			size_t index = 0;
			for (auto& item : value) {
				const string item_name = lf::format("[{}]", index);
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
				return error(generic_errc::parse_error, detail::context_message(stream, lf::format("reading {} failed: vector size {} exceeds limit {}", detail::value_type_name<Vector>(), item_count.value, stream.limits().max_vector_elements)));
			}
			if constexpr (requires { stream.add_progress_total(item_count.value); }) {
				stream.add_progress_total(item_count.value);
			}

			vector_t temp;
			if (auto err = detail::validate_vector_allocation(stream, temp, item_count.value); err) {
				return err;
			}
			temp.reserve(static_cast<size_t>(item_count.value));
			for (size_t index = 0; index < static_cast<size_t>(item_count.value); ++index) {
				temp.emplace_back();
				const string item_name = lf::format("[{}]", index);
				if (auto err = stream(field(item_name, temp.back(), args...)); err) {
					return err;
				}
			}
			value = std::move(temp);
			return {};
		}
	}

	template<byte_stream Stream, typename Optional, typename... Args>
		requires(specialized_data<Optional, optional> && sizeof...(Args) > 0)
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
			if constexpr (requires { stream.add_progress_total(value.size()); }) {
				stream.add_progress_total(value.size());
			}
			size_t index = 0;
			for (const auto& [key, mapped] : value) {
				const string item_name = lf::format("[{}]", index);
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
				return error(generic_errc::parse_error, detail::context_message(stream, lf::format("reading {} failed: map size {} exceeds limit {}", detail::value_type_name<Map>(), item_count.value, stream.limits().max_vector_elements)));
			}
			if constexpr (requires { stream.add_progress_total(item_count.value); }) {
				stream.add_progress_total(item_count.value);
			}
			map_t temp;
			temp.reserve(static_cast<size_t>(item_count.value));
			for (size_t index = 0; index < static_cast<size_t>(item_count.value); ++index) {
				key_t key{};
				mapped_t mapped{};
				const string item_name = lf::format("[{}]", index);
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
		requires(data<Array, std::remove_cvref_t<Array>> && is_array<std::remove_cvref_t<Array>>::value)
	error process(Stream& stream, Array& value) {
		using element_t = typename std::remove_cvref_t<Array>::value_type;
		if constexpr (detail::bulk_binary_element_v<element_t>) {
			detail::context_scope scope(stream, "data");
			return detail::process_bulk_elements(stream, span(value.data(), value.size()));
		}

		if constexpr (requires { stream.add_progress_total(value.size()); }) {
			stream.add_progress_total(value.size());
		}
		size_t index = 0;
		for (auto& item : value) {
			const string item_name = lf::format("[{}]", index);
			if (auto err = stream(field(item_name, item)); err) {
				return err;
			}
			++index;
		}
		return {};
	}

	template<byte_stream Stream, typename Array, typename... Args>
		requires(data<Array, std::remove_cvref_t<Array>> && is_array<std::remove_cvref_t<Array>>::value && sizeof...(Args) > 0)
	error process(Stream& stream, Array& value, Args&... args) {
		if constexpr (requires { stream.add_progress_total(value.size()); }) {
			stream.add_progress_total(value.size());
		}
		size_t index = 0;
		for (auto& item : value) {
			const string item_name = lf::format("[{}]", index);
			if (auto err = stream(field(item_name, item, args...)); err) {
				return err;
			}
			++index;
		}
		return {};
	}

	template<byte_stream Stream, glm_vec2_data Vec2>
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

	/*!
	** @ingroup binary
	** @brief Parses a complete byte span into T.
	**
	** read() fails if parsing fails or if any trailing bytes remain. Use
	** read_stream directly when the input span contains more data after T.
	*/
	template<typename T>
	report<T> read(span<const lf::byte> bytes, read_options options = {}) {
		T value{};
		read_stream stream{bytes, options.limits};
		stream.set_progress(options.progress);
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		if (stream.cursor() != bytes.size()) {
			return unexpected(error(generic_errc::parse_error, detail::context_message(stream, lf::format("reading {} failed: {} trailing byte(s) remain", detail::value_type_name<T>(), bytes.size() - stream.cursor()))));
		}
		return value;
	}

	/*!
	** @ingroup binary
	** @brief Parses a complete byte span into T with reduced diagnostics.
	**
	** fast_read() uses fast_read_stream. It emits the same values as read() but
	** omits field context from parse errors.
	*/
	template<typename T>
	report<T> fast_read(span<const lf::byte> bytes, read_options options = {}) {
		T value{};
		fast_read_stream stream{bytes, options.limits};
		stream.set_progress(options.progress);
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		if (stream.cursor() != bytes.size()) {
			return unexpected(error(generic_errc::parse_error, lf::format("reading {} failed: {} trailing byte(s) remain", detail::value_type_name<T>(), bytes.size() - stream.cursor())));
		}
		return value;
	}

	/*!
	** @ingroup binary
	** @brief Alias for read<T>().
	*/
	template<typename T>
	report<T> process(span<const lf::byte> bytes, read_options options = {}) {
		return read<T>(bytes, options);
	}

	/*!
	** @ingroup binary
	** @brief Parses a complete graph payload and resolves deferred ref<T> values.
	*/
	template<typename T>
	report<T> read_graph(span<const lf::byte> bytes, read_options options = {}) {
		T value{};
		read_stream stream{bytes, options.limits};
		stream.set_progress(options.progress);
		if (options.byte_progress) {
			stream.set_byte_progress(std::move(options.byte_progress));
		}
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		if (auto err = stream.refs().run_fixups(); err) {
			return unexpected(err);
		}
		if (stream.cursor() != bytes.size()) {
			return unexpected(error(generic_errc::parse_error, detail::context_message(stream, lf::format("reading {} failed: {} trailing byte(s) remain", detail::value_type_name<T>(), bytes.size() - stream.cursor()))));
		}
		return value;
	}

	/*!
	** @ingroup binary
	** @brief Computes the number of bytes write(value) would produce.
	*/
	template<typename T>
	report<size_t> measure(const T& value) {
		measure_stream stream;
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		return stream.size();
	}

	/*!
	** @ingroup binary
	** @brief Serializes value into an owned byte vector.
	*/
	template<typename T>
	report<vector<lf::byte>> write(const T& value, write_options options = {}) {
		write_stream stream;
		stream.set_progress(options.progress);
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		return stream.take_written();
	}

	/*!
	** @ingroup binary
	** @brief Serializes value with reduced diagnostic context overhead.
	**
	** fast_write() emits the same bytes as write().
	*/
	template<typename T>
	report<vector<lf::byte>> fast_write(const T& value, write_options options = {}) {
		fast_write_stream stream;
		stream.set_progress(options.progress);
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		return stream.take_written();
	}

	/*!
	** @ingroup binary
	** @brief Serializes a graph payload and validates all non-null ref<T> targets.
	*/
	template<typename T>
	report<vector<lf::byte>> write_graph(const T& value, write_options options = {}) {
		write_stream stream;
		stream.set_progress(options.progress);
		if (options.byte_progress) {
			stream.set_byte_progress(std::move(options.byte_progress), options.byte_progress_total);
		}
		if (auto err = process(stream, value); err) {
			return unexpected(err);
		}
		if (auto err = stream.refs().validate(); err) {
			return unexpected(err);
		}
		return stream.take_written();
	}

	/*!
	** @ingroup binary
	** @brief Serializes value into caller-provided storage.
	**
	** Returns an error if the output span cannot hold the complete encoded
	** value. Use measure(value) to size the span first when needed.
	*/
	template<typename T>
	error write_to(span<lf::byte> out, const T& value, write_options options = {}) {
		fixed_write_stream stream{out};
		stream.set_progress(options.progress);
		return process(stream, value);
	}
} // namespace lf::bin
