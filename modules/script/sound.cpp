#include "leaf/script/prototypes/sound.hpp"

#include "leaf/application/sound.hpp"
#include "leaf/core/format.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/script/virtual_filesystem.hpp"

namespace lf {
	SoundPrototype::SoundPrototype(const dict& data) : Prototype(data) {
		load_field(data, "path", path);
		if (has_field(data, "sound_type")) {
			load_field(data, "sound_type", sound_type);
		}
		if (has_field(data, "volume")) {
			load_field(data, "volume", volume);
		}
	}

	SoundPrototype::~SoundPrototype() = default;

	error SoundPrototype::load() {
		if (asset || path.empty()) {
			return {};
		}
		auto resolved = ResolveVirtualPathReport(path);
		if (!resolved) {
			log::Warning("{}", lf::format("[sound] {}", resolved.error().message));
			return {};
		}
		auto loaded = LoadSoundAsset(*resolved);
		if (!loaded) {
			log::Warning("{}", lf::format("[sound] {}", loaded.error().message));
			return {};
		}
		asset = std::move(*loaded);
		return {};
	}
} // namespace lf
