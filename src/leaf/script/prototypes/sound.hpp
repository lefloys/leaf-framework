#pragma once

#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include "leaf/script/prototype.hpp"

namespace lf {
	struct SoundPrototype : public Prototype<identifier<SoundPrototype, u16, void>> {
		string path;
		string sound_type = "effects";
		f32 volume = 1.0f;

		explicit SoundPrototype(const dict& data);

		static constexpr string_view type() noexcept { return "sound"; }
	};
}
