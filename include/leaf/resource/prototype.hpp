#pragma once

#include <leaf/core/dynamic_object.hpp>
#include <leaf/core/identifier.hpp>
#include <leaf/core/string.hpp>
#include <leaf/resource/database.hpp>
#include <leaf/script/local_string.hpp>

namespace lf {
	template<typename T, typename VNum>
	struct object_trait<identifier<T, VNum, void>> {
		static identifier<T, VNum, void> parse(const object& value) {
			return Database<T>::find(value.as<string>());
		}
	};

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
