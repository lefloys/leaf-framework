#pragma once

#include <span>

namespace lf {
	template<typename T, std::size_t Extent = std::dynamic_extent>
	struct span : public std::span<T, Extent> {
		using std::span<T, Extent>::span;
	};
}
