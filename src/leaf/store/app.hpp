#pragma once

#include <leaf/core/string.hpp>

namespace lf::store {
	struct App {
		string id;

		static App Current();
		static App Get(string_view id);

		explicit operator bool() const noexcept {
			return !id.empty();
		}
	};
}
