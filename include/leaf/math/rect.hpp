#pragma once

#include "leaf/core/types.hpp"
#include "leaf/math/dim.hpp"
#include "leaf/math/pos.hpp"

namespace lf {
	template<typename T>
	struct rect {
		pos2<T> pos;
		dim2<T> dim;
	};
} // namespace lf
