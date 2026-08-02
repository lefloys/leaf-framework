#include "leaf/script/prototypes/cursor.hpp"

#include "leaf/core/format.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/platform/platform.hpp"
#include "leaf/script/virtual_filesystem.hpp"

#include <stb_image.h>

#include <memory>

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

	error CursorPrototype::load() {
		if (handle || path.empty()) {
			return {};
		}

		auto resolved = ResolveVirtualPathReport(path);
		if (!resolved) {
			log::Warning("{}", lf::format("[cursor] {}", resolved.error().message));
			return {};
		}

		int width = 0;
		int height = 0;
		int components = 0;
		std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels{ stbi_load(resolved->string().c_str(), &width, &height, &components, 4), stbi_image_free };
		if (!pixels || width <= 0 || height <= 0) {
			log::Warning("{}", lf::format("[cursor] failed to load cursor image '{}'", resolved->string()));
			return {};
		}

		if (hotspot_x >= static_cast<u32>(width) || hotspot_y >= static_cast<u32>(height)) {
			log::Warning("{}", lf::format("[cursor] hotspot outside cursor image '{}'", Database<CursorPrototype>::name(id)));
			return {};
		}

		handle = create_platform_cursor(pixels.get(), static_cast<u32>(width), static_cast<u32>(height), hotspot_x, hotspot_y);
		return {};
	}

	CursorPrototype::~CursorPrototype() {
		destroy_platform_cursor(handle);
	}
}
