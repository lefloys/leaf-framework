#pragma once
#include "database.hpp"
#include "leaf/core/dynamic_object.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/vector.hpp"
#include "prototype_inspection.hpp"

#include <functional>

namespace lf {
	struct PrototypeTypeFunctions {
		std::function<void()> clear;
		std::function<void(string_view, const dict&)> create;
		std::function<string_view()> type;
		std::function<void()> resolve;
		std::function<size_t()> count;
		std::function<string_view(size_t)> name;
		std::function<PrototypeFieldList(size_t)> runtime_fields;
	};

	template <typename T>
	PrototypeFieldList inspect_runtime_fields(const T&);

	struct PrototypeTypeRegistry {
		template <typename T>
		static void RegisterType() {
			auto& funcs = functions.emplace_back();
			using db = Database<T>;
			funcs.type = db::type;
			funcs.clear = []() { db::prototypes.clear(); };
			funcs.create = db::create;
			funcs.resolve = []() { db::resolve_connectors(); };
			funcs.count = []() { return db::prototypes.size(); };
			funcs.name = [](size_t index) -> string_view { return db::prototypes[index].name; };
			funcs.runtime_fields = [](size_t index) { return inspect_runtime_fields(db::prototypes[index]); };
		}

		inline static vector<PrototypeTypeFunctions> functions = {};
	};
} // namespace lf
