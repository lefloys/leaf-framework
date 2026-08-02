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
	class safe_cast_exception : public std::runtime_error {
	  public:
		using std::runtime_error::runtime_error;
	};

	[[noreturn]] void rethrow_with_context(string_view context);
} // namespace lf
