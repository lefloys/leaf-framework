#include "cursor.hpp"

#include <leaf/core/format.hpp>
#include <leaf/core/messages.hpp>
#include <leaf/script/cursor_prototype.hpp>
#include <leaf/script/database.hpp>
#include <leaf/script/virtual_filesystem.hpp>

#include <memory>

#include <stb_image.h>

namespace lf {
	bool SetCursorPrototype(view<window> display, string_view name) {
		if (!display) {
			return false;
		}
		if (name.empty() || name == "default") {
			Window::ResetCursor(display);
			return true;
		}

		identifier<CursorPrototype, u16, void> id = Database<CursorPrototype>::find(name);
		if (!id) {
			log_warning(format("[cursor] missing cursor prototype '{}'", name));
			return false;
		}

		const CursorPrototype& cursor = Database<CursorPrototype>::get(id);
		auto path = ResolveVirtualPathReport(cursor.path);
		if (!path) {
			log_warning(format("[cursor] {}", path.error().message));
			return false;
		}

		int width = 0;
		int height = 0;
		int components = 0;
		std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
			stbi_load(path->string().c_str(), &width, &height, &components, 4),
			stbi_image_free);
		if (!pixels || width <= 0 || height <= 0) {
			log_warning(format("[cursor] failed to load cursor image '{}'", path->string()));
			return false;
		}

		if (cursor.hotspot_x >= static_cast<u32>(width) || cursor.hotspot_y >= static_cast<u32>(height)) {
			log_warning(format("[cursor] hotspot outside cursor image '{}'", cursor.name));
			return false;
		}

		Window::SetCursor(
			display,
			pixels.get(),
			static_cast<u32>(width),
			static_cast<u32>(height),
			cursor.hotspot_x,
			cursor.hotspot_y);
		return true;
	}

	void InstallCursorScript(sol::state& lua, view<window> display) {
		lua.set_function("set_cursor", [display](string_view name) {
			return SetCursorPrototype(display, name);
		});
	}
}
