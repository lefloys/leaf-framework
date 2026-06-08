#pragma once

#include <leaf/core/types.hpp>

namespace lf {
	template <typename T>
	struct rect {
		T left{};
		T top{};
		T width{};
		T height{};
	};
} // namespace lf
