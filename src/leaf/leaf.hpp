
#pragma once

#include "leaf/core/types.hpp"
#include "leaf/core/error.hpp"

namespace lf {
	error Init(i32 argc, char* argv[]);
	void Exit();
}