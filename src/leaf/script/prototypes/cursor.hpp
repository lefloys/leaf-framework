#pragma once

#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include "leaf/script/prototype.hpp"

namespace lf {
	struct PlatformCursor;

	struct CursorPrototype final : public Prototype<identifier<CursorPrototype, u16, void>> {
		string path;
		u32 hotspot_x = 0;
		u32 hotspot_y = 0;
		PlatformCursor* handle = nullptr;

		explicit CursorPrototype(const dict& data);
		~CursorPrototype();

		error load() override;

		static constexpr string_view type() noexcept { return "cursor"; }
	};
}
