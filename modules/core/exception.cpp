#include "leaf/core/exception.hpp"

#include "leaf/core/format.hpp"

namespace lf {
	void rethrow_with_context(string_view context) {
		try {
			throw;
		} catch (const std::exception& e) {
			throw std::runtime_error(lf::format("{}\n -> {}", context, e.what()));
		} catch (...) {
			throw std::runtime_error(lf::format("{}\n -> unknown exception", context));
		}
	}
} // namespace lf
