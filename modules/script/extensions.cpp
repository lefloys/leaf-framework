#include "leaf/script/extensions.hpp"

namespace lf {
	void script_system::register_installer(script_extension extension) {
		instance().extensions.push_back(extension);
	}

	void script_system::install(sol::state& state) {
		for (script_extension extension : instance().extensions) {
			extension(state);
		}
	}
}
