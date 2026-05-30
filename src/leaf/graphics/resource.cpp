#include "resource.hpp"

#include <leaf/core/exception.hpp>

#include <iostream>
namespace lf {

	void detail::check_rutile_error(string_view context) {
		if (rtError() == RT_SUCCESS) {
			return;
		}

		string message(context);
		const char* rutile_message = rtErrorMessage();
		if (rutile_message[0]) {
			message += ": ";
			message += rutile_message;
		}
		std::cout << rtError() << " " << rutile_message << "\n";
		rtClearError();
		throw runtime_exception(message);
	}

} // namespace lf
