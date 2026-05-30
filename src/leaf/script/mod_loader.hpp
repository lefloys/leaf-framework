#pragma once

#include "leaf/core/error.hpp"
#include "leaf/script/localization.hpp"
#include "leaf/script/mod_info.hpp"

#include <functional>

namespace lf {
	struct SceneOption {
		string path;
	};

	struct ModOptions {
		SceneOption main_scene;
		string language = string(default_language);
		vector<ModInfo> mods;
	};

	struct ModLoadProgress {
		std::function<void(string stage, f32 stage_progress, string process, f32 process_progress)> report;
	};

	// Loads all mods within the mod tree returning any error that occurs.
	error LoadMods(ModCollection& mod_tree, ModLoadProgress* progress = nullptr);
	const ModOptions& LoadedModOptions();
	const vector<ModInfo>& LoadedMods();
	// Clears all prototypes
	void UnloadMods();
} // namespace lf
