#pragma once

#include <leaf/core/dynamic_object.hpp>
#include <leaf/core/format.hpp>
#include <leaf/core/identifier.hpp>
#include <leaf/core/string.hpp>
#include <leaf/resource/database.hpp>
#include <leaf/script/local_string.hpp>

namespace lf {
	struct PrototypeBase {
		PrototypeBase(const dict& data) {
			string name;
			data.assign(
				field("name", name),
				field("local_name", local_name, name),
				field("local_description", local_description, local_name),
				field("order", order, "1")
			);
		}
		virtual ~PrototypeBase() = default;

		string order = "1";
		local_string local_name;
		local_string local_description;

	  protected:
		bool has_field(const dict& data, string_view field_name) {
			return data.find(field_name) != data.end();
		}

		template<typename T>
		void load_field(const dict& data, string_view field_name, T& out) {
			const auto iterator = data.find(field_name);
			if (iterator == data.end()) {
				throw runtime_exception(lf::format("missing field '{}'", field_name));
			}
			const object& object_value = iterator->second;
			if (object_value.convertible<T>()) {
				out = object_value.as<T>();
				return;
			}
			out = data.parse_field<T>(field_name);
		}

		template<typename T>
		void load_field(const list& data, size_t index, T& out) {
			if (index >= data.size()) {
				throw runtime_exception(lf::format("index {} out of range", index));
			}
			const object& object_value = data[index];
			if (object_value.convertible<T>()) {
				out = object_value.as<T>();
				return;
			}
			out = data.parse_index<T>(index);
		}
	};

	template<typename T>
	struct Prototype : public PrototypeBase {
		Prototype(const dict& data) : PrototypeBase(data) {}
		virtual ~Prototype() = default;
		using ID = T;
		ID id;

		virtual error load() {
			return {};
		};
	};
} // namespace lf
