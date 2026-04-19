#pragma once

#include <expected>

namespace lf {
	template<typename T, typename E>
	using expected = std::expected<T, E>;

	using std::unexpected;
}
