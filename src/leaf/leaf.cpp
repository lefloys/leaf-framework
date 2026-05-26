

#include "leaf.hpp"

#include "leaf/core/error.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"
#include "leaf/launcher/launcher.hpp"
#include "leaf/graphics/window.hpp"
#include "leaf/application/rml_backend.hpp"
#include "leaf/system/system.hpp"
#include "leaf/platform/platform.hpp"
#include "leaf/graphics/graphics.hpp"

namespace lf {
	error Init(span<string_view> args) {
		error err;

		err = init_system(args);
		if (err) { goto system_exit; }

		err = init_platform(args);
		if (err) { goto platform_exit; }

		err = init_graphics(args);
		if (err) { goto graphics_exit; }

		err = init_launcher(args);
		if (err) { goto launcher_exit; }

		err = init_rml(args); 
		if (err) { goto rml_exit; }

		return err;
	rml_exit:
		exit_rml();
	launcher_exit:
		exit_launcher();
	graphics_exit:
		exit_graphics();
	platform_exit:
		exit_platform();
	system_exit:
		exit_system();
		return err;
	}

	bool Update() {
		update_launcher();
		return update_platform();
	}

	void Exit() {
		exit_rml();
		exit_launcher();
		exit_graphics();
		exit_platform();
		exit_system();
	}
} // namespace lf
