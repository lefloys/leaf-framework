#include "cursor.hpp"

#include "leaf/core/format.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/script/database.hpp"
#include "leaf/script/prototypes/cursor.hpp"

namespace lf {
	bool SetCursorPrototype(view<window> display, string_view name) {
		if (!display) {
			return false;
		}
		if (name.empty() || name == "default") {
			Window::ApplyCursor(display, {}, nullptr);
			return true;
		}

		auto id = Database<CursorPrototype>::find(name);
		if (!id) {
			log::Warning("{}", lf::format("[cursor] missing cursor prototype '{}'", name));
			return false;
		}
		const CursorPrototype& cursor = Database<CursorPrototype>::get(id);
		if (!cursor.handle) {
			log::Warning("{}", lf::format("[cursor] cursor prototype '{}' not loaded", name));
			return false;
		}
		Window::ApplyCursor(display, name, cursor.handle);
		return true;
	}
}
