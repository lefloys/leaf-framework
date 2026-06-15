#include "dynamic_object.hpp"

#include <yaml-cpp/yaml.h>

#include <stdexcept>

namespace lf {

	object::object() : object_underlying(std::monostate{}) {}
	object::object(bool value) : object_underlying(value) {}
	object::object(i08 value) : object_underlying(static_cast<i64>(value)) {}
	object::object(i16 value) : object_underlying(static_cast<i64>(value)) {}
	object::object(i32 value) : object_underlying(static_cast<i64>(value)) {}
	object::object(i64 value) : object_underlying(static_cast<i64>(value)) {}
	object::object(u08 value) : object_underlying(static_cast<u64>(value)) {}
	object::object(u16 value) : object_underlying(static_cast<u64>(value)) {}
	object::object(u32 value) : object_underlying(static_cast<u64>(value)) {}
	object::object(u64 value) : object_underlying(value) {}
	object::object(f64 value) : object_underlying(value) {}
	object::object(const char* value) : object_underlying(string(value)) {}
	object::object(const string& value) : object_underlying(value) {}
	object::object(string_view value) : object_underlying(string(value)) {}
	object::object(const dict& value) : object_underlying(value) {}
	object::object(const list& value) : object_underlying(value) {}

	object& object::at(string_view key) {
		if (!is<dict>()) {
			throw std::runtime_error(
				lf::format("object::at: not a dict (type = {})", current_type_name()));
		}
		return get<dict>().at(string(key));
	}

	const object& object::at(string_view key) const {
		if (!is<dict>()) {
			throw std::runtime_error(
				lf::format("object::at: not a dict (type = {})", current_type_name()));
		}
		return get<dict>().at(string(key));
	}

	object& object::at(size_t index) {
		if (!is<list>()) {
			throw std::runtime_error(
				lf::format("object::at: not a list (type = {})", current_type_name()));
		}
		return get<list>().at(index);
	}

	const object& object::at(size_t index) const {
		if (!is<list>()) {
			throw std::runtime_error(
				lf::format("object::at: not a list (type = {})", current_type_name()));
		}
		return get<list>().at(index);
	}

	object& object::operator[](string_view key) {
		return get<dict>()[string(key)];
	}
	object& object::operator[](size_t index) {
		return get<list>()[index];
	}
	const object& object::operator[](size_t index) const {
		return get<list>()[index];
	}

	string_view object::current_type_name() const {
		return std::visit(
			[](auto&& v) -> string_view {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<T, std::monostate>) {
					return "null";
				} else if constexpr (std::is_same_v<T, bool>) {
					return "bool";
				} else if constexpr (std::is_same_v<T, i64>) {
					return "i64";
				} else if constexpr (std::is_same_v<T, u64>) {
					return "u64";
				} else if constexpr (std::is_same_v<T, f64>) {
					return "f64";
				} else if constexpr (std::is_same_v<T, string>) {
					return "string";
				} else if constexpr (std::is_same_v<T, dict>) {
					return "dict";
				} else if constexpr (std::is_same_v<T, list>) {
					return "list";
				}
			},
			*static_cast<const object_underlying*>(this));
	}

	void EmitYaml(YAML::Emitter& out, const object& value) {
		value.visit([&out](const auto& typed) {
			using T = std::decay_t<decltype(typed)>;
			if constexpr (std::is_same_v<T, std::monostate>) {
				out << YAML::Null;
			} else if constexpr (std::is_same_v<T, dict>) {
				out << YAML::BeginMap;
				for (const auto& [key, child] : typed) {
					out << YAML::Key << key;
					out << YAML::Value;
					EmitYaml(out, child);
				}
				out << YAML::EndMap;
			} else if constexpr (std::is_same_v<T, list>) {
				out << YAML::BeginSeq;
				for (const object& child : typed) {
					EmitYaml(out, child);
				}
				out << YAML::EndSeq;
			} else {
				out << typed;
			}
		});
	}

	object ObjectFromYaml(const YAML::Node& node) {
		if (node.IsNull()) {
			return {};
		}
		if (node.IsMap()) {
			dict result;
			for (const auto& entry : node) {
				result[entry.first.as<string>()] = ObjectFromYaml(entry.second);
			}
			return result;
		}
		if (node.IsSequence()) {
			list result;
			for (const auto& entry : node) {
				result.push_back(ObjectFromYaml(entry));
			}
			return result;
		}
		try {
			return object(node.as<bool>());
		} catch (const YAML::Exception&) {}
		try {
			return object(node.as<i64>());
		} catch (const YAML::Exception&) {}
		try {
			return object(node.as<u64>());
		} catch (const YAML::Exception&) {}
		try {
			return object(node.as<f64>());
		} catch (const YAML::Exception&) {}
		return object(node.as<string>());
	}

} // namespace lf

