#include "leaf/script/state.hpp"

#include "leaf/script/extensions.hpp"

namespace lf {
	sol::state CreateState() {
		sol::state state;
		state.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
		script_system::install(state);
		return state;
	}

	void PrepareState(sol::state& state, span<const script_installer> installers) {
		for (const script_installer& install : installers) {
			install(state);
		}
	}
}
