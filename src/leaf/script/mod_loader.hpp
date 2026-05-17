#pragma once

#include "leaf/core/error.hpp"
#include "leaf/script/mod_info.hpp"

#include <functional>

namespace lf {
	struct ModLoadProgress {
		std::function<void(string stage, f32 stage_progress, string process, f32 process_progress)> report;
	};

	// Loads all mods within the mod tree returning any error that occurs.
	error LoadMods(ModCollection& mod_tree, ModLoadProgress* progress = nullptr);
	// Clears all prototypes
	void UnloadMods();
} // namespace lf
