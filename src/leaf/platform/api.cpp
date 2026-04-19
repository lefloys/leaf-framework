#include "api.hpp"

namespace lf {
	PlatformAPI Platform{};
	void SetPlatformAPI(PlatformAPI api) {
		Platform = api;
	}
}
