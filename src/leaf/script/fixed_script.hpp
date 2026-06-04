#pragma once

#include "leaf/core/fixed.hpp"

#include <sol/sol.hpp>

namespace lf {
	report<fixed> FixedFromLua(sol::object value);
	void InstallFixedScript(sol::state& lua, bool install_global_coercion);
} // namespace lf
