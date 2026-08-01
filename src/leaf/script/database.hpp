#pragma once

#include "leaf/core/dynamic_object.hpp"
#include "leaf/core/error.hpp"
#include "leaf/core/identifier.hpp"
#include "leaf/core/vector.hpp"

namespace lf {
	/*!
	** @brief
	*/
	template <typename T>
	struct Database {
		static vector<T> prototypes;
		static void create(string_view name, const dict& data);
		static void clear();
		static void reserve(size_t count);
		static size_t count();
		static string_view name(size_t index);
		static void load_assets();
		static identifier<T, u16, void> find(string_view name);
		static T& get(identifier<T, u16, void> id);
		static const T& get(identifier<const T, u16, void> id);
		static string_view type();
		static void resolve_connectors();

		static const T& get_null();
	};
	template <typename T>
	vector<T> Database<T>::prototypes = {};
	template <typename T>
	void Database<T>::create(string_view name, const dict& data) {
		T& it = prototypes.emplace_back(data);
		it.name = name;
		it.finalize_base_fields(data);
		identifier<T, u16, void> idx(static_cast<u16>(prototypes.size() - 1));
		it.id = idx;
		it.finalize_localization();
	}

	template <typename T>
	void Database<T>::clear() {
		prototypes.clear();
	}

	template <typename T>
	void Database<T>::reserve(size_t count) {
		prototypes.reserve(prototypes.size() + count);
	}

	template <typename T>
	size_t Database<T>::count() {
		return prototypes.size();
	}

	template <typename T>
	string_view Database<T>::name(size_t index) {
		return prototypes[index].name;
	}

	template <typename T>
	void Database<T>::load_assets() {
		for (T& prototype : prototypes) {
			prototype.load();
		}
	}
	template <typename T>
	identifier<T, u16, void> Database<T>::find(string_view name) {
		for (u16 i = 0; i < prototypes.size(); ++i) {
			if (prototypes[i].name == name) {
				return identifier<T, u16, void>(i);
			}
		}
		return identifier<T, u16, void>();
	}
	template <typename T>
	T& Database<T>::get(identifier<T, u16, void> id) {
		if (id.get() >= prototypes.size()) {
			throw runtime_exception(lf::format("{} prototype id {} out of range", type(), id.get()));
		}
		return prototypes[id.get()];
	}
	template <typename T>
	const T& Database<T>::get(identifier<const T, u16, void> id) {
		if (id.get() >= prototypes.size()) {
			throw runtime_exception(lf::format("{} prototype id {} out of range", type(), id.get()));
		}
		return prototypes[id.get()];
	}
	template <typename T>
	string_view Database<T>::type() {
		return T::type();
	}
	template <typename T>
	void Database<T>::resolve_connectors() {
		for (size_t i = 1; i < prototypes.size(); ++i) {
			prototypes[i].resolve_connectors();
		}
	}

	template <typename T>
	const T& Database<T>::get_null() {
		if (prototypes.empty()) {
			throw runtime_exception(lf::format("{} null prototype is missing", type()));
		}
		return prototypes[0];
	}
} // namespace lf

