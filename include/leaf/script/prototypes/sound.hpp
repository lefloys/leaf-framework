#pragma once

#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include "leaf/script/prototype.hpp"

#include <memory>

namespace lf {
	struct SoundAsset;

	struct SoundPrototype final : public Prototype<identifier<SoundPrototype, u16, void>> {
		static constexpr string_view type() noexcept { return "sound"; }

		explicit SoundPrototype(const dict& data);
		~SoundPrototype();
		error load() override;

		string path;
		string sound_type = "effects";
		f32 volume = 1.0f;
		std::shared_ptr<const SoundAsset> asset;
	};
} // namespace lf
