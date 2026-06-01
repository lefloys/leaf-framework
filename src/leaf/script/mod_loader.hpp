#pragma once

#include "leaf/core/error.hpp"
#include "leaf/script/localization.hpp"
#include "leaf/script/mod_info.hpp"

#include <functional>

namespace lf {
	/*!
	** @ingroup modding
	** @brief Scene entry point requested by loaded mod options.
	*/
	struct SceneOption {
		string path;
	};

	/*!
	** @ingroup modding
	** @brief Runtime options assembled from loaded mod metadata.
	*/
	struct ModOptions {
		SceneOption main_scene;
		string language = string(default_language);
		vector<ModInfo> mods;
	};

	/*!
	** @ingroup modding
	** @brief Optional progress callback used while loading mods.
	*/
	struct ModLoadProgress {
		std::function<void(string stage, f32 stage_progress, string process, f32 process_progress)> report;
	};

	/*!
	** @ingroup modding
	** @brief Loads all mods in the supplied mod collection.
	** @param mod_tree Privileged and unprivileged mod roots to scan.
	** @param progress Optional progress reporter.
	** @return An error if loading fails, or an empty error on success.
	*/
	error LoadMods(ModCollection& mod_tree, ModLoadProgress* progress = nullptr);

	/*!
	** @ingroup modding
	** @brief Gets the options produced by the most recent successful mod load.
	*/
	const ModOptions& LoadedModOptions();

	/*!
	** @ingroup modding
	** @brief Gets the mods produced by the most recent successful mod load.
	*/
	const vector<ModInfo>& LoadedMods();

	/*!
	** @ingroup modding
	** @brief Clears loaded mods and registered prototypes.
	*/
	void UnloadMods();
} // namespace lf
