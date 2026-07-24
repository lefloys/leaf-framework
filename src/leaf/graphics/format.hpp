#ifndef LEAF_GRAPHICS_FORMAT_HPP
#define LEAF_GRAPHICS_FORMAT_HPP

#include <leaf/graphics/resource.hpp>

namespace rt {
	// @GPT : This is crazy. Why arent you setting them equal to rutile's value. like "Unkown = RT_FORMAT_UNKNOWN" etc.
	enum class Format {
		Unknown,
		Rg32Float,
		Rgb32Float,
		Rgba32Float,
	};

} // namespace rt

#endif /* LEAF_GRAPHICS_FORMAT_HPP */
