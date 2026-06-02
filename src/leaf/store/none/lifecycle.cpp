#include "leaf/store/lifecycle.hpp"

#include <leaf/logging/logging.hpp>

namespace lf {


	error init_store(span<string_view>) {
		log::Debug("Store backend: none");
		return error::no_error;
	}

	void update_store() {
	}

	void exit_store() {}

} // namespace lf
