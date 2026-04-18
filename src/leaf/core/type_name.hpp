#pragma once

#include "string.hpp"

namespace lf {
	template<typename T>
	struct type_name_trait;

	template<typename T>
	constexpr string_view type_name() {
		return type_name_trait<T>::value;
	}
}
