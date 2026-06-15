#include "resource.hpp"

#include "leaf/core/exception.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/graphics/graphics.hpp"

namespace lf {

	void detail::check_rutile_error(string_view context) {

		error err = rutile_error();
		if (!err) { return; }
		rtClearError();

		throw runtime_exception(lf::format("{} : {}", context, err.message));
	}

} // namespace lf

