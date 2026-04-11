
#include "leaf.hpp"

#include <iostream>
#include <string>
#include <vector>

#include <Windows.h>

namespace lf {
	error Init(i32 argc, char* argv[]) {
		std::cout << "Hello leaf!\n";
		return error::no_error;
	}
	void Exit() {


	}
}