#pragma once

#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include "leaf/script/prototype.hpp"

#include <memory>

namespace lf {
	struct SoundAsset;

	struct SoundPrototype : public Prototype<identifier<SoundPrototype, u16, void>>, public AssetPrototype {
		string path;
		string sound_type = "effects";
		f32 volume = 1.0f;
		std::shared_ptr<const SoundAsset> asset;

		explicit SoundPrototype(const dict& data);
		void load_asset() override;
		void unload_asset() override;

		static constexpr string_view type() noexcept { return "sound"; }
	};
}
