#pragma once
#include "database.hpp"
#include "leaf/core/dynamic_object.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/vector.hpp"

#include <functional>

namespace lf {
	struct PrototypeTypeFunctions {
		std::function<void()> clear;
		std::function<void(string_view, const dict&)> create;
		std::function<string_view()> type;
		std::function<void()> resolve;
	};

	struct PrototypeTypeRegistry {
		template <typename T>
		static void RegisterType() {
			auto& funcs = functions.emplace_back();
			using db = Database<T>;
			funcs.type = db::type;
			funcs.clear = []() { db::prototypes.clear(); };
			funcs.create = db::create;
			funcs.resolve = []() { db::resolve_connectors(); };
		}

		inline static vector<PrototypeTypeFunctions> functions = {};
	};
} // namespace lf
