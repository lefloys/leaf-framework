#pragma once

#include <vector>

namespace lf {
	template<typename T, typename Alloc = std::allocator<T>>
	struct vector : public std::vector<T, Alloc> {
		using std::vector<T, Alloc>::vector;
	};
}