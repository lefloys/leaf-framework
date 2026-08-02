#include "leaf/graphics/window.hpp"

#include "leaf/core/format.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/resource/database.hpp"

namespace lf {
	bool SetCursorPrototype(rt::view<rt::window> display, CursorPrototype::ID id) {
		if (!display) {
			return false;
		}
		if (!id) {
			rt::Window::ApplyCursor(display, {}, nullptr);
			return true;
		}
		const CursorPrototype& cursor = Database<CursorPrototype>::get(id);
		if (!cursor.handle) {
			log::Warning("{}", lf::format("[cursor] cursor prototype '{}' not loaded", Database<CursorPrototype>::name(id)));
			return false;
		}
		rt::Window::ApplyCursor(display, Database<CursorPrototype>::name(id), reinterpret_cast<rt::PlatformCursor*>(cursor.handle));
		return true;
	}
} // namespace lf
