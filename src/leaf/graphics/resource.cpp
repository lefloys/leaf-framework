#include "resource.hpp"

#include "leaf/core/exception.hpp"
#include "leaf/logging/logging.hpp"
#include "leaf/graphics/graphics.hpp"

namespace lf {

	void detail::check_rutile_error(string_view context) {

		error err = to_error(rtError());
		if (!err) { return; }
		rtClearError();

		

		throw runtime_exception(format("{} : {}", context, err);
	}

} // namespace lf
