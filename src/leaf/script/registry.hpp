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
		void (*create)(string_view, const dict&) = nullptr;
		string_view (*type)() = nullptr;
		void (*resolve)() = nullptr;
		void (*reserve)(size_t) = nullptr;
		size_t (*count)() = nullptr;
		string_view (*name)(size_t) = nullptr;
		void (*load_assets)() = nullptr;
		void (*unload_assets)() = nullptr;
	};

	struct PrototypeTypeRegistry {
		template<typename T>
		static void RegisterType() {
			auto& funcs = functions.emplace_back();
			using db = Database<T>;
			funcs.type = db::type;
			funcs.clear = []() { db::prototypes.clear(); };
			funcs.create = [](string_view name, const dict& data) { (void) db::create(name, data); };
			funcs.resolve = &db::resolve_connectors;
			funcs.reserve = [](size_t count) { db::prototypes.reserve(db::prototypes.size() + count); };
			funcs.count = []() { return db::prototypes.size(); };
			funcs.name = [](size_t index) -> string_view { return db::prototypes[index].name; };
			if constexpr (std::is_base_of_v<AssetPrototype, T>) {
				funcs.load_assets = []() {
					for (T& p : db::prototypes) {
						static_cast<AssetPrototype&>(p).load_asset();
					}
				};
				funcs.unload_assets = []() {
					for (T& p : db::prototypes) {
						static_cast<AssetPrototype&>(p).unload_asset();
					}
				};
			}
		}

		inline static vector<PrototypeTypeFunctions> functions = {};
	};

	inline void LoadAssetPrototypes() {
		for (const PrototypeTypeFunctions& funcs : PrototypeTypeRegistry::functions) {
			if (funcs.load_assets) {
				funcs.load_assets();
			}
		}
	}

	inline void UnloadAssetPrototypes() {
		for (const PrototypeTypeFunctions& funcs : PrototypeTypeRegistry::functions) {
			if (funcs.unload_assets) {
				funcs.unload_assets();
			}
		}
	}
} // namespace lf
