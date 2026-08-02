#pragma once
#include "leaf/core/dynamic_object.hpp"
#include "leaf/core/exception.hpp"
#include "leaf/core/format.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/resource/database.hpp"
#include "leaf/resource/prototype.hpp"

#include <type_traits>

namespace lf {
	struct PrototypeTypeFunctions {
		void (*clear)() = nullptr;
		void (*create)(string_view) = nullptr;
		void (*init)(string_view, const dict&) = nullptr;
		string_view (*type)() = nullptr;
		size_t (*count)() = nullptr;
		string_view (*name)(size_t) = nullptr;
		void (*load_assets)() = nullptr;
	};

	struct PrototypeTypeRegistry {
		template<typename T>
		static void RegisterType() {
			using db = Database<T>;
			static const auto name = [](size_t index) {
				return db::name(typename T::ID{ static_cast<typename T::ID::vnum_t>(index + 1) });
			};
			functions.push_back({ &db::clear,
								  &db::create,
								  &db::init,
								  &db::type,
								  &db::count,
								  name,
								  &db::load_assets });
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
