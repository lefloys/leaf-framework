#ifndef LEAF_GRAPHICS_TIMEPOINT_HPP
#define LEAF_GRAPHICS_TIMEPOINT_HPP

#include <leaf/graphics/resource.hpp>
// @GPT : ffs namespaces ive said it so many times
namespace rt {
	namespace Timepoint {
		void Wait(timepoint timepoint);
		bool Reached(timepoint timepoint);
	} // namespace Timepoint
} // namespace rt

#endif /* LEAF_GRAPHICS_TIMEPOINT_HPP */
