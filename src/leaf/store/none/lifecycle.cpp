#include "leaf/store/lifecycle.hpp"

#include <iostream>

namespace lf {


	error init_store(span<string_view>) {
		std::cout << "store : none\n";
		return error::no_error;
	}

	void update_store() {
	}

	void exit_store() {}

} // namespace lf
