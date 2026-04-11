#pragma once

#include "leaf/core/types.hpp"


namespace lf {
	template<typename T>
	struct dim2 {
		T width;
		T height;
	};
	template<typename T>
	struct dim3 {
		T width;
		T height;
		T depth;
	};

}