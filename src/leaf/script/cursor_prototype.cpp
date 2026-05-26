#include "cursor_prototype.hpp"

namespace lf {
	CursorPrototype::CursorPrototype(const dict& data) : Prototype(data) {
		load_field(data, "path", path);
		if (has_field(data, "hotspot_x")) {
			load_field(data, "hotspot_x", hotspot_x);
		}
		if (has_field(data, "hotspot_y")) {
			load_field(data, "hotspot_y", hotspot_y);
		}
	}
}
