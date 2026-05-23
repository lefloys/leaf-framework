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
		static identifier<T, u16, void> create(string_view name, const dict& data);
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
	identifier<T, u16, void> Database<T>::create(string_view name, const dict& data) {
		T& it = prototypes.emplace_back(data);
		it.name = name;
		identifier<T, u16, void> idx(static_cast<u16>(prototypes.size() - 1));
		it.id = idx;
		it.finalize_localization();
		return idx;
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
		return prototypes[id.get()];
	}
	template <typename T>
	const T& Database<T>::get(identifier<const T, u16, void> id) {
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
		return prototypes[0];
	}
} // namespace lf
