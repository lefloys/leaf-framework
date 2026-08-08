#pragma once

#include "leaf/core/types.hpp"

#include <random>

namespace lf {
	// Returns a process-local seed sourced from std::random_device. The standard
	// does not guarantee that random_device is backed by physical entropy.
	u64 random_seed();
} // namespace lf
