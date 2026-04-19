
#include "leaf.hpp"
#include "graphics/detail/api.hpp"
#include "platform/api.hpp"

namespace lf {
	error Init(i32 argc, char* argv[]) {
		if (!Platform.init) {
			return error(generic_errc::missing_field, "no platform backend selected; call lf::SetPlatformAPI(...) before lf::Init()");
		}

		if (error err = Platform.init()) {
			return err;
		}

		if (error err = Graphics.init()) {
			Platform.exit();
			return err;
		}

		return error::no_error;
	}

	void Exit() {
		Graphics.exit();
		Platform.exit();
	}
}
