#include "leaf/application/window.hpp"

#include "leaf/core/format.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/resource/database.hpp"

namespace lf {
	bool SetCursorPrototype(Window& display, CursorPrototype::ID id) {
		if (!id) {
			display.set_cursor({}, nullptr);
			return true;
		}
		const CursorPrototype& cursor = Database<CursorPrototype>::get(id);
		if (!cursor.handle) {
			log::Warning("{}", lf::format("[cursor] cursor prototype '{}' not loaded", Database<CursorPrototype>::name(id)));
			return false;
		}
		display.set_cursor(Database<CursorPrototype>::name(id), reinterpret_cast<rt::PlatformCursor*>(cursor.handle));
		return true;
	}
} // namespace lf
