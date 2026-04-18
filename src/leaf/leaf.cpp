
#include "leaf.hpp"
#include "graphics/detail/api.hpp"
#include "platform/window.hpp"

namespace lf {
	error Init(i32 argc, char* argv[]) {
		(void)argc;
		(void)argv;
		if (error err = detail::platform_init()) {
			return err;
		}

		if (error err = detail::graphics_init()) {
			detail::platform_exit();
			return err;
		}

		return error::no_error;
	}

	void Exit() {
		detail::graphics_exit();
		detail::platform_exit();
	}
}
