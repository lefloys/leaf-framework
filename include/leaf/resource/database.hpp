#pragma once

#include "leaf/core/dynamic_object.hpp"
#include "leaf/core/error.hpp"
#include "leaf/core/indexed_map.hpp"
#include "leaf/core/vector.hpp"

namespace lf {
	template<typename T>
	struct Database {
		static vector<T> prototypes;
		static indexed_map<string> names;
		static void create(string_view name);
		static void init(string_view name, const dict& data);
		static void clear();
		static size_t count();
		static string_view name(typename T::ID id);
		static void load_assets();
		static typename T::ID find(string_view name);
		static T& get(typename T::ID id);
		static string_view type();
	};

	template<typename T> vector<T> Database<T>::prototypes = {};
	template<typename T> indexed_map<string> Database<T>::names = {};

	template<typename T> void Database<T>::create(string_view name) {
		if (!names.insert(string(name)).second) {
			throw runtime_exception(lf::format("duplicate {} prototype '{}'", type(), name));
		}
	}

	template<typename T> void Database<T>::init(string_view name, const dict& data) {
		const auto name_it = names.find(string(name));
		if (name_it == names.end()) {
			throw runtime_exception(lf::format("{} prototype '{}' was not created", type(), name));
		}
		const size_t index = names.index_of(name_it);
		if (index != prototypes.size()) {
			throw runtime_exception(lf::format("{} prototype '{}' was initialized out of creation order", type(), name));
		}
		T& prototype = prototypes.emplace_back(data);
		prototype.id = typename T::ID{ static_cast<typename T::ID::vnum_t>(index + 1) };
	}

	template<typename T> void Database<T>::clear() { prototypes.clear(); names.clear(); }
	template<typename T> size_t Database<T>::count() { return prototypes.size(); }

	template<typename T> string_view Database<T>::name(typename T::ID id) {
		if (!id || id.get() > prototypes.size()) {
			throw runtime_exception(lf::format("{} prototype id {} is invalid", type(), id.get()));
		}
		return names[static_cast<size_t>(id.get() - 1)];
	}

	template<typename T> void Database<T>::load_assets() {
		for (T& prototype : prototypes) {
			prototype.load();
		}
	}

	template<typename T> typename T::ID Database<T>::find(string_view name) {
		using id_type = typename T::ID;
		const auto it = names.find(string(name));
		return it == names.end() ? id_type{} : id_type{ static_cast<typename id_type::vnum_t>(names.index_of(it) + 1) };
	}

	template<typename T> T& Database<T>::get(typename T::ID id) {
		if (!id || id.get() > prototypes.size()) {
			throw runtime_exception(lf::format("{} prototype id {} out of range", type(), id.get()));
		}
		return prototypes[id.get() - 1];
	}

	template<typename T> string_view Database<T>::type() { return T::type(); }
} // namespace lf
