#include "api.hpp"

namespace lf {
	PlatformAPI Platform{};
	void SetPlatformAPI(PlatformAPI api) {
		Platform = api;
	}

	bool has_platform_backend() {
		return Platform.init;
	}
}
