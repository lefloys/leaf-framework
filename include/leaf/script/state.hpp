#pragma once

#include <leaf/core/span.hpp>

#include <sol/sol.hpp>

#include <functional>

namespace Rml {
	class ElementDocument;
}

namespace lf {
	using script_installer = std::function<void(sol::state&, Rml::ElementDocument&)>;

	sol::state CreateState();
	void PrepareState(sol::state& state, Rml::ElementDocument& document, span<const script_installer> installers);
}
