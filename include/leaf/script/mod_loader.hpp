#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/progress.hpp"
#include "leaf/core/yaml.hpp"
#include "leaf/script/localization.hpp"
#include "leaf/script/mod_info.hpp"

#include <sol/sol.hpp>

namespace lf {
	/*!
	** @ingroup modding
	** @brief Loads all mods in the supplied mod collection.
	** @param mod_tree Privileged and unprivileged mod roots to scan.
	** @param progress Shared progress state used by the loader.
	** @return An error if loading fails, or an empty error on success.
	*/
	error LoadMods(ModCollection& mod_tree, Progress& progress);

	/*!
	** @ingroup modding
	** @brief Gets the raw option table produced by the most recent successful mod load.
	*/
	const object& LoadedModOptions();

	/*!
	** @ingroup modding
	** @brief Gets the raw option table produced by the most recent successful mod load.
	*/
	const vector<ModInfo>& LoadedMods();
	object sol_to_object(const sol::object& value);

	/*!
	** @ingroup modding
	** @brief Clears loaded mods and registered prototypes.
	*/
	void UnloadMods();
} // namespace lf
