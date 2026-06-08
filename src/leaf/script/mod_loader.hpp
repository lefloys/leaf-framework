#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/dynamic_object.hpp"
#include "leaf/script/localization.hpp"
#include "leaf/script/mod_info.hpp"

#include <functional>

namespace lf {
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
	** @brief Gets the raw option table produced by the most recent successful mod load.
	*/
	const object& LoadedModOptions();

	/*!
	** @ingroup modding
	** @brief Gets the main menu scene path selected by loaded mod data.
	*/
	string_view LoadedMainScenePath();

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
