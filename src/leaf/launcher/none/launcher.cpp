#include "leaf/launcher/launcher.hpp"

#include <iostream>

namespace lf {
	error init_launcher(span<string_view>) {
		std::cout << "launcher : none\n";
		return error::no_error;
	}



	void update_launcher() {
	}

	void exit_launcher() {}
} // namespace lf
