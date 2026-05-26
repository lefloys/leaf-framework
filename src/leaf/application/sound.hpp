#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/filesystem.hpp>
#include <leaf/core/types.hpp>

#include <sol/sol.hpp>

namespace lf {
	error PlaySoundFile(const fs::path& path, f32 volume);
	void InstallSoundScript(sol::state& lua);
}
