#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/filesystem.hpp>
#include <leaf/core/types.hpp>

#include <sol/sol.hpp>

#include <memory>

namespace lf {
	struct SoundAsset;
	using SoundAssetHandle = std::shared_ptr<const SoundAsset>;

	report<SoundAssetHandle> LoadSoundAsset(const fs::path& path);
	error PlaySoundAsset(const SoundAssetHandle& asset, f32 volume);
	error PlaySoundFile(const fs::path& path, f32 volume);
	void InstallSoundScript(sol::state& lua);
}
