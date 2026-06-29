#pragma once

#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include "leaf/script/prototype.hpp"

namespace lf {
	struct PlatformCursor;

	struct CursorPrototype : public Prototype<identifier<CursorPrototype, u16, void>>, public AssetPrototype {
		string path;
		u32 hotspot_x = 0;
		u32 hotspot_y = 0;
		PlatformCursor* handle = nullptr;

		explicit CursorPrototype(const dict& data);

		void load_asset() override;
		void unload_asset() override;

		static constexpr string_view type() noexcept { return "cursor"; }
	};
}
