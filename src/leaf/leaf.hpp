
#pragma once

#include "leaf/core/types.hpp"
#include "leaf/core/error.hpp"

namespace Setting {
#ifdef LEAF_DEBUG
	constexpr bool DEBUG = true;
#else
	constexpr bool DEBUG = false;
#endif
}

namespace lf {
	error Init();
	void Exit();
	bool Update();
}
