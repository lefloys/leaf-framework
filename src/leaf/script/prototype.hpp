#pragma once

#include <leaf/core/dynamic_object.hpp>
#include <leaf/core/format.hpp>
#include <leaf/core/identifier.hpp>
#include <leaf/core/string.hpp>
#include <leaf/script/database.hpp>
#include <leaf/script/local_string.hpp>

#include <optional>

namespace lf {
	template <typename T>
	using PrototypeID = identifier<T, u16, void>;

	struct PrototypeBase {
		PrototypeBase(const dict& data) {
			if (has_field(data, "local_name")) {
				load_field(data, "local_name", local_name);
			}
			if (has_field(data, "local_description")) {
				load_field(data, "local_description", local_description);
			}
		}
		virtual ~PrototypeBase() = default;
		virtual void resolve_connectors() {}

		string name = "null";
		string order = "1";
		local_string local_name;
		local_string local_description;

		void finalize_localization() {
			if (local_name.key.empty()) {
				local_name.key = name;
			}
			if (local_description.key.empty()) {
				local_description.key = local_name.key;
			}
		}

		void finalize_base_fields(const dict& data) {
			if (has_field(data, "order")) {
				load_field(data, "order", order);
			}
		}

	  protected:
		bool has_field(const dict& data, string_view field_name) {
			return data.find(field_name) != data.end();
		}

		template <typename T>
		void load_field(const dict& data, string_view field_name, T& out) {
			auto it = data.find(field_name);
			if (it == data.end()) {
				throw runtime_exception(lf::format("missing field '{}'", field_name));
			}
			const object& obj = it->second;
			if (obj.convertible<T>()) {
				out = obj.as<T>();
				return;
			}
			out = data.parse_field<T>(field_name);
		}

		template <typename T>
		void load_field(const list& data, size_t index, T& out) {
			if (index >= data.size()) {
				throw runtime_exception(lf::format("index {} out of range", index));
			}
			const object& obj = data[index];
			if (obj.convertible<T>()) {
				out = obj.as<T>();
				return;
			}
			out = data.parse_index<T>(index);
		}
	};

	template <typename Proto>
	struct PrototypeConnector {
		using id_t = PrototypeID<Proto>;

		string name;
		id_t id;

		void resolve(string_view owner_name, string_view field_name, bool required = false, std::optional<id_t> fallback = {}) {
			if (name.empty()) {
				if (required) {
					throw runtime_exception(lf::format("{} missing required field '{}'", owner_name, field_name));
				}
				id = fallback.value_or(id_t(0));
				return;
			}
			id = Database<Proto>::find(name);
			if (!id) {
				throw runtime_exception(lf::format("{} references missing {} '{}'", owner_name, Proto::type(), name));
			}
		}
	};

	template <typename T>
	struct Prototype : public PrototypeBase {
		Prototype(const dict& data) : PrototypeBase(data) {}
		using id_t = T;
		id_t id;
	};
} // namespace lf

