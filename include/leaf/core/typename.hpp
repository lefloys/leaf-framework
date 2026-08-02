#pragma once

namespace lf {
	template<typename T>
	struct type_name_trait {
		static constexpr const char* get() {
			return "undefined typename";
		}
	};
	template<typename T>
	constexpr const char* type_name() {
		return type_name_trait<T>::get();
	}
	template<typename T>
	constexpr const char* type_name(const T&) {
		return type_name<T>();
	}
	template<>
	struct type_name_trait<bool> {
		static constexpr const char* get() {
			return "bool";
		}
	};
	template<>
	struct type_name_trait<const char*> {
		static constexpr const char* get() {
			return "const char*";
		}
	};
} // namespace lf
