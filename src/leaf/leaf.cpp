#include "leaf.hpp"

#include "leaf/application/rml_backend.hpp"
#include "leaf/core/error.hpp"
#include "leaf/core/filesystem.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"
#include "leaf/graphics/graphics.hpp"
#include "leaf/graphics/window.hpp"
#include "leaf/platform/platform.hpp"
#include "leaf/store/lifecycle.hpp"
#include "leaf/system/system.hpp"

namespace lf {
	error Init(span<string_view> args) {
		error err;

		log::Info("[leaf] initializing leaf-framework...");

		err = init_system(args);
		if (err) { goto system_exit; }

		err = init_graphics(args, false);
		if (err) { goto graphics_exit; }

		err = init_store(args);
		if (err) { goto store_exit;	}

		err = init_platform(args);
		if (err) { goto platform_exit; }

		err = init_rml(args);
		if (err) { goto rml_exit; }

		return err;
	rml_exit:
		exit_rml();
	store_exit:
		exit_store();
	graphics_exit:
		exit_graphics();
	platform_exit:
		exit_platform();
	system_exit:
		exit_system();
		log::Error("{}", err.message);
		return err;
	}


	error InitHeadless(span<string_view> args) {
		error err;

		err = init_system(args);
		if (err) { goto system_exit; }

		err = init_graphics(args, true);
		if (err) { goto graphics_exit; }

		err = init_store(args);
		if (err) { goto store_exit; }

		return err;
	store_exit:
		exit_store();
	graphics_exit:
		exit_graphics();
	system_exit:
		exit_system();
		log::Error("{}", err.message);
		return err;
	}

	bool Update() {
		update_store();
		return update_platform();
	}

	void Exit() {
		log::Debug("[leaf] Shutting down!");

		exit_rml();
		exit_platform();

		exit_store();
		exit_graphics();
		exit_system();
	}

	void ExitHeadless() {
		log::Debug("[leaf] Shutting down!");

		exit_store();
		exit_graphics();
		exit_system();
	}
} // namespace lf
