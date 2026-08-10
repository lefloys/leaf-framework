#include "leaf/script/state.hpp"

#include "leaf/script/extensions.hpp"

namespace lf {
	sol::state CreateState() {
		sol::state state{};
		state.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
		InstallSoundScript(state);
		return state;
	}
}
