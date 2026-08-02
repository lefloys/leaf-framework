#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include <exception>
#include <stdexcept>

namespace lf {

	using std::exception;
	using out_of_range_exception = std::out_of_range;
	using runtime_exception = std::runtime_error;
	using invalid_argument_exception = std::invalid_argument;

	[[noreturn]] void rethrow_with_context(string_view context);
} // namespace lf
