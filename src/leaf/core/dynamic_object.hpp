#pragma once
#include "error.hpp"
#include "exception.hpp"
#include "string.hpp"
#include "typename.hpp"
#include "types.hpp"
#include "unordered_map.hpp"
#include "vector.hpp"
#include <limits>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

namespace YAML {
	class Emitter;
	class Node;
}

namespace lf {

	template <typename T, typename Variant>
	struct is_in_variant;

	template <typename T, typename... Ts>
	struct is_in_variant<T, std::variant<Ts...>> : std::disjunction<std::is_same<T, Ts>...> {};

	template <typename T, typename Variant>
	constexpr bool is_in_variant_v = is_in_variant<T, Variant>::value;

	class dict;
	class list;
	class object;
	using dict_underlying = unordered_map_string<object>;
	using list_underlying = vector<object>;
	using object_underlying = std::variant<std::monostate, bool, i64, u64, f64, string, dict, list>;

	class dict : public dict_underlying {
	  public:
		template <typename T>
		T parse_field(string_view index) const;
	};
	class list : public vector<object> {
	  public:
		template <typename T>
		T parse_index(size_t index) const;
	};

	class object : object_underlying {
	  public:
		using value_type = object_underlying;
		object();
		object(bool value);
		object(i08 value);
		object(i16 value);
		object(i32 value);
		object(i64 value);
		object(u08 value);
		object(u16 value);
		object(u32 value);
		object(u64 value);
		object(f64 value);
		object(const char* value);
		object(string_view value);
		object(const string& value);
		object(const dict& value);
		object(const list& value);

		template <typename T>
			requires is_in_variant_v<T, object::value_type>
		bool is() const;

		template <typename T>
		bool convertible() const;

		template <typename T>
			requires is_in_variant_v<T, object::value_type>
		T& get();

		template <typename T>
			requires is_in_variant_v<T, object::value_type>
		const T& get() const;

		string_view current_type_name() const;

		object& at(string_view key);
		const object& at(string_view key) const;

		object& at(size_t index);
		const object& at(size_t index) const;

		object& operator[](string_view key);
		const object& operator[](string_view key) const;

		object& operator[](size_t index);
		const object& operator[](size_t index) const;

		template <typename T>
		T as() const;

		template <typename T>
		T parse() const;

		template <typename Visitor>
		decltype(auto) visit(Visitor&& visitor) const;
	};

	template <typename T>
	struct object_trait {
		static T parse(const object& obj) = delete;
		static dict to_dict(const T& value) = delete;
	};

	template <typename T>
	T dict::parse_field(string_view field) const {
		auto it = this->find(field);
		if (it == this->end()) {
			throw lf::error(lf::generic_errc::missing_field,
							lf::format("missing field '{}'", field));
		}
		try {
			return object_trait<T>::parse(it->second);
		} catch (...) {
			rethrow_with_context(lf::format("field '{}' to struct '{}'", field, type_name<T>()));
		}
	}

	template <typename T>
	T list::parse_index(size_t index) const {
		try {
			return object_trait<T>::parse(at(index));
		} catch (...) {
			rethrow_with_context(
				lf::format("when parsing list index {} to '{}'", index, typeid(T).name()));
		}
	}

	template <typename T>
		requires is_in_variant_v<T, object::value_type>
	bool object::is() const {
		return std::holds_alternative<T>(*static_cast<const object_underlying*>(this));
	}

	template <typename T>
	bool object::convertible() const {
		if constexpr (std::is_same_v<T, bool>) {
			return is<bool>();
		} else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
			return is<i64>() || is<u64>() || is<f64>();
		} else if constexpr (std::is_same_v<T, string>) {
			return is<string>();
		} else {
			return false;
		}
	}

	template <typename T>
		requires is_in_variant_v<T, object::value_type>
	T& object::get() {
		return std::get<T>(*static_cast<object_underlying*>(this));
	}

	template <typename T>
		requires is_in_variant_v<T, object::value_type>
	const T& object::get() const {
		return std::get<T>(*static_cast<const object_underlying*>(this));
	}

	template <typename T>
	inline T object::as() const {
		if (!convertible<T>()) {
			throw std::bad_variant_access();
		}
		if constexpr (std::is_same_v<T, bool>) {
			return get<bool>();
		} else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
			if (is<i64>()) {
				return static_cast<T>(get<i64>());
			}
			if (is<u64>()) {
				return static_cast<T>(get<u64>());
			}
			if (is<f64>()) {
				return static_cast<T>(get<f64>());
			}
		} else if constexpr (std::is_same_v<T, string>) {
			return get<string>();
		}
		throw std::bad_variant_access();
	}

	template <typename T>
	T object::parse() const {
		return object_trait<T>::parse(*this);
	}

	template <typename Visitor>
	decltype(auto) object::visit(Visitor&& visitor) const {
		return std::visit(std::forward<Visitor>(visitor), *static_cast<const object_underlying*>(this));
	}

	void EmitYaml(YAML::Emitter& out, const object& value);
	object ObjectFromYaml(const YAML::Node& node);
} // namespace lf

namespace lf {
	template <>
	struct type_name_trait<dict> {
		static constexpr const char* get() {
			return "dict";
		}
	};
	template <>
	struct type_name_trait<list> {
		static constexpr const char* get() {
			return "list";
		}
	};
	template <>
	struct type_name_trait<object> {
		static constexpr const char* get() {
			return "object";
		}
	};

	template <>
	struct object_trait<f32> {
		static f32 parse(const object& obj) {
			if (obj.is<f64>()) {
				f64 d = obj.get<f64>();
				if (d < static_cast<f64>(std::numeric_limits<f32>::lowest()) ||
					d > static_cast<f64>(std::numeric_limits<f32>::max())) {
					throw lf::runtime_exception(lf::format("value {} out of range for f32", d));
				}
				return static_cast<f32>(d);
			} else if (obj.is<i64>()) {
				i64 i = obj.get<i64>();
				if (i < static_cast<i64>(std::numeric_limits<f32>::lowest()) ||
					i > static_cast<i64>(std::numeric_limits<f32>::max())) {
					throw lf::runtime_exception(lf::format("value {} out of range for f32", i));
				}
				return static_cast<f32>(i);
			} else if (obj.is<u64>()) {
				u64 u = obj.get<u64>();
				if (u > static_cast<u64>(std::numeric_limits<f32>::max())) {
					throw lf::runtime_exception(lf::format("value {} out of range for f32", u));
				}
				return static_cast<f32>(u);
			} else {
				throw lf::runtime_exception(
					lf::format("cannot convert type '{}' to f32", obj.current_type_name()));
			}
		}
	};
	template <>
	struct object_trait<f64> {
		static f64 parse(const object& obj) {
			if (obj.is<f64>()) {
				return obj.get<f64>();
			} else if (obj.is<i64>()) {
				i64 i = obj.get<i64>();
				return static_cast<f64>(i);
			} else if (obj.is<u64>()) {
				u64 u = obj.get<u64>();
				return static_cast<f64>(u);
			} else {
				throw lf::runtime_exception(
					lf::format("cannot convert type '{}' to f64", obj.current_type_name()));
			}
		}
	};
	template <>
	struct object_trait<bool> {
		static bool parse(const object& obj) {
			if (obj.is<bool>()) {
				return obj.get<bool>();
			}
			throw lf::runtime_exception(
				lf::format("cannot convert type '{}' to bool", obj.current_type_name()));
		}
	};

	template <>
	struct object_trait<i64> {
		static i64 parse(const object& obj) {
			if (obj.is<i64>()) {
				return obj.get<i64>();
			}
			if (obj.is<u64>()) {
				u64 v = obj.get<u64>();
				if (v > static_cast<u64>(std::numeric_limits<i64>::max())) {
					throw lf::runtime_exception(lf::format("value {} out of range for i64", v));
				}
				return static_cast<i64>(v);
			}
			if (obj.is<f64>()) {
				return static_cast<i64>(obj.get<f64>());
			}
			throw lf::runtime_exception(
				lf::format("cannot convert type '{}' to i64", obj.current_type_name()));
		}
	};
	template <>
	struct object_trait<u64> {
		static u64 parse(const object& obj) {
			if (obj.is<u64>()) {
				return obj.get<u64>();
			}
			if (obj.is<i64>()) {
				i64 v = obj.get<i64>();
				if (v < 0) {
					throw lf::runtime_exception(lf::format("value {} out of range for u64", v));
				}
				return static_cast<u64>(v);
			}
			if (obj.is<f64>()) {
				f64 v = obj.get<f64>();
				if (v < 0 || v > static_cast<f64>(std::numeric_limits<u64>::max())) {
					throw lf::runtime_exception(lf::format("value {} out of range for u64", v));
				}
				return static_cast<u64>(v);
			}
			throw lf::runtime_exception(
				lf::format("cannot convert type '{}' to u64", obj.current_type_name()));
		}
	};

	template <>
	struct object_trait<i32> {
		static i32 parse(const object& obj) {
			i64 v = object_trait<i64>::parse(obj);
			if (v < static_cast<i64>(std::numeric_limits<i32>::min()) ||
				v > static_cast<i64>(std::numeric_limits<i32>::max())) {
				throw lf::runtime_exception(lf::format("value {} out of range for i32", v));
			}
			return static_cast<i32>(v);
		}
	};
	template <>
	struct object_trait<u32> {
		static u32 parse(const object& obj) {
			u64 v = object_trait<u64>::parse(obj);
			if (v > static_cast<u64>(std::numeric_limits<u32>::max())) {
				throw lf::runtime_exception(lf::format("value {} out of range for u32", v));
			}
			return static_cast<u32>(v);
		}
	};
	template <>
	struct object_trait<i16> {
		static i16 parse(const object& obj) {
			i64 v = object_trait<i64>::parse(obj);
			if (v < static_cast<i64>(std::numeric_limits<i16>::min()) ||
				v > static_cast<i64>(std::numeric_limits<i16>::max())) {
				throw lf::runtime_exception(lf::format("value {} out of range for i16", v));
			}
			return static_cast<i16>(v);
		}
	};
	template <>
	struct object_trait<u16> {
		static u16 parse(const object& obj) {
			u64 v = object_trait<u64>::parse(obj);
			if (v > static_cast<u64>(std::numeric_limits<u16>::max())) {
				throw lf::runtime_exception(lf::format("value {} out of range for u16", v));
			}
			return static_cast<u16>(v);
		}
	};
	template <>
	struct object_trait<i08> {
		static i08 parse(const object& obj) {
			i64 v = object_trait<i64>::parse(obj);
			if (v < static_cast<i64>(std::numeric_limits<i08>::min()) ||
				v > static_cast<i64>(std::numeric_limits<i08>::max())) {
				throw lf::runtime_exception(lf::format("value {} out of range for i08", v));
			}
			return static_cast<i08>(v);
		}
	};
	template <>
	struct object_trait<u08> {
		static u08 parse(const object& obj) {
			u64 v = object_trait<u64>::parse(obj);
			if (v > static_cast<u64>(std::numeric_limits<u08>::max())) {
				throw lf::runtime_exception(lf::format("value {} out of range for u08", v));
			}
			return static_cast<u08>(v);
		}
	};

	template <>
	struct object_trait<string> {
		static string parse(const object& obj) {
			if (obj.is<string>()) {
				return obj.get<string>();
			}
			throw lf::runtime_exception(
				lf::format("cannot convert type '{}' to string", obj.current_type_name()));
		}
	};
	template <>
	struct object_trait<dict> {
		static dict parse(const object& obj) {
			if (obj.is<dict>()) {
				return obj.get<dict>();
			}
			throw lf::runtime_exception(
				lf::format("cannot convert type '{}' to dict", obj.current_type_name()));
		}
	};
	template <>
	struct object_trait<list> {
		static list parse(const object& obj) {
			if (obj.is<list>()) {
				return obj.get<list>();
			}
			throw lf::runtime_exception(
				lf::format("cannot convert type '{}' to list", obj.current_type_name()));
		}
	};
} // namespace lf

