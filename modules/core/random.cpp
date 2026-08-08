#include "leaf/core/random.hpp"

namespace lf {
	u64 random_seed() {
		std::random_device device;
		return std::uniform_int_distribution<u64>{}(device);
	}
} // namespace lf
