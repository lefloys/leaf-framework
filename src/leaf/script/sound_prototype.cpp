#include "sound_prototype.hpp"

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
}
