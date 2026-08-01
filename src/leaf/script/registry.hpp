#pragma once
#include "database.hpp"
#include "leaf/core/exception.hpp"
#include "leaf/core/format.hpp"
#include "leaf/core/dynamic_object.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/script/prototype.hpp"

#include <type_traits>

namespace lf {
	struct PrototypeTypeFunctions {
		void (*clear)() = nullptr;
		void (*register_name)(string_view) = nullptr;
		void (*create)(string_view, const dict&) = nullptr;
		string_view (*type)() = nullptr;
		void (*reserve)(size_t) = nullptr;
		size_t (*count)() = nullptr;
		string_view (*name)(size_t) = nullptr;
		void (*load_assets)() = nullptr;
	};

	struct PrototypeTypeRegistry {
		template<typename T>
		static void RegisterType() {
			using db = Database<T>;
			functions.push_back({
				&db::clear,
				&db::register_name,
				&db::create,
				&db::type,
				&db::reserve,
				&db::count,
				&db::name,
				&db::load_assets
			});
		}

		inline static vector<PrototypeTypeFunctions> functions = {};
	};

	inline void LoadAssetPrototypes() {
		for (const PrototypeTypeFunctions& functions : PrototypeTypeRegistry::functions) {
			if (functions.load_assets) {
				functions.load_assets();
			}
		}
	}
} // namespace lf
