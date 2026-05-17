#include "exception.hpp"

#include "format.hpp"

namespace lf {
	void rethrow_with_context(string_view context) {
		try {
			throw;
		} catch (const std::exception& e) {
			throw std::runtime_error(format("{}\n -> {}", context, e.what()));
		} catch (...) {
			throw std::runtime_error(format("{}\n -> unknown exception", context));
		}
	}
} // namespace lf