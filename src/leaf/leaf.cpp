
#include "leaf.hpp"
#include "graphics/detail/api.hpp"
#include "platform/api.hpp"

namespace lf {
	error Init() {
		if (has_platform_backend()) {
			if (error err = Platform.init()) {
				return err;
			}
		}

		if (error err = Graphics.init()) {
			if (Platform.exit) {
				Platform.exit();
			}
			return err;
		}

		return error::no_error;
	}

	void Exit() {
		Graphics.exit();
		if (Platform.exit) {
			Platform.exit();
		}
	}

	bool Update() {
		if (has_platform_backend() && Platform.poll_events) {
			Platform.poll_events();
		}

		if (has_platform_backend() && Platform.any_window_should_close) {
			return !Platform.any_window_should_close();
		}

		return true;
	}
}
