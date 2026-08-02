#ifndef LEAF_GRAPHICS_FORMAT_HPP
#define LEAF_GRAPHICS_FORMAT_HPP

#include <leaf/graphics/resource.hpp>

namespace rt {
	enum class Format : u32 {
		Unknown = 0,
		Rg32Float = 14,
		Rgb32Float = 15,
		Rgba32Float = 16,
	};

} // namespace rt

#endif /* LEAF_GRAPHICS_FORMAT_HPP */
