#pragma once

#include <leaf/core/dynamic_object.hpp>
#include <leaf/core/format.hpp>
#include <leaf/core/string.hpp>

namespace lf {
	struct local_string {
		string key;
	};

	template <>
	struct type_name_trait<local_string> {
		static constexpr const char* get() {
			return "local_string";
		}
	};

	template <>
	struct object_trait<local_string> {
		static local_string parse(const object& obj) {
			if (obj.is<string>()) {
				return { obj.get<string>() };
			}
			throw lf::runtime_exception(format("cannot convert type '{}' to local_string", obj.current_type_name()));
		}
	};
} // namespace lf
