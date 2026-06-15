#pragma once

#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include "leaf/script/prototype.hpp"

namespace lf {
	struct CursorPrototype : public Prototype<identifier<CursorPrototype, u16, void>> {
		string path;
		u32 hotspot_x = 0;
		u32 hotspot_y = 0;

		explicit CursorPrototype(const dict& data);

		static constexpr string_view type() noexcept { return "cursor"; }
	};
}
