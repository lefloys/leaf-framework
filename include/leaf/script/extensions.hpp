#pragma once

#include <leaf/core/vector.hpp>
#include <leaf/core/singleton.hpp>

#include <sol/sol.hpp>

namespace lf {
	using script_extension = void (*)(sol::state&);

	struct script_system : Singleton<script_system> {
		vector<script_extension> extensions;

		static void register_installer(script_extension extension);
		static void install(sol::state& state);
	};

}
