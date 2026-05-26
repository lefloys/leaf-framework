#pragma once

#include <leaf/graphics/window.hpp>

#include <sol/sol.hpp>

namespace lf {
	bool SetCursorPrototype(view<window> display, string_view name);
	void InstallCursorScript(sol::state& lua, view<window> display);
}
