#pragma once

#include <leaf/core/span.hpp>

#include <sol/sol.hpp>

#include <functional>

namespace lf {
	using script_installer = std::function<void(sol::state&)>;

	sol::state CreateState();
	void PrepareState(sol::state& state, span<const script_installer> installers);
}
