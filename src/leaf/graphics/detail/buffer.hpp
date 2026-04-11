#pragma once

#include "resource.hpp"

namespace lf::detail {
	template<typename T>
	struct Buffer : Resource<T> {
	};
}