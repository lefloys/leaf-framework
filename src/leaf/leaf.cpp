

#include "leaf.hpp"

#include "leaf/core/error.hpp"
#include "leaf/core/filesystem.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"
#include "leaf/logging/logging.hpp"
#include "leaf/store/lifecycle.hpp"
#include "leaf/graphics/window.hpp"
#include "leaf/application/rml_backend.hpp"
#include "leaf/system/system.hpp"
#include "leaf/platform/platform.hpp"
#include "leaf/graphics/graphics.hpp"

namespace lf {
	void ConfigureUserDataRoot(const InitOptions& options) {
		if (options.portable_user_data) {
			OverwriteAppdataDir((fs::path(GetInstallDir()) / "").string());
		} else if (!options.appdata_folder_name.empty()) {
			OverwriteAppdataDir((fs::path(GetAppdataDir()) / string(options.appdata_folder_name)).string());
		}
	}

	void LogRuntimeInfo(bool headless) {
		log::Info("[leaf] system: {}", system_backend_name());
		if (!headless) {
			log::Info("[leaf] platform: {}", platform_backend_name());
		}
		log::Info("[leaf] store: {}", store_backend_name());
		log::Info("[leaf] install: {}", GetInstallDir());
		log::Info("[leaf] appdata: {}", GetAppdataDir());
	}

	error Init(span<string_view> args) {
		return Init(args, {});
	}

	error Init(span<string_view> args, InitOptions options) {
		error err = init_logging(args);
		if (err) { return err; }

		err = init_system(args);
		if (err) { log::Error("{}", err.message); goto system_exit; }
		ConfigureUserDataRoot(options);
		log::Debug("system initialized");

		err = init_platform(args);
		if (err) { log::Error("{}", err.message); goto platform_exit; }
		log::Debug("platform initialized");

		err = init_graphics(args);
		if (err) { log::Error("{}", err.message); goto graphics_exit; }
		log::Debug("graphics initialized");

		err = init_store(args);
		if (err) { log::Error("{}", err.message); goto store_exit; }
		log::Debug("store initialized");

		err = init_rml(args); 
		if (err) { log::Error("{}", err.message); goto rml_exit; }

		LogRuntimeInfo(false);
		LogGraphicsInfo();
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
		return err;
	}

	error InitHeadless(span<string_view> args) {
		return InitHeadless(args, {});
	}

	error InitHeadless(span<string_view> args, InitOptions options) {
		error err = init_logging(args);
		if (err) { return err; }

		err = init_system(args);
		if (err) { log::Error("{}", err.message); goto logging_exit; }
		ConfigureUserDataRoot(options);
		log::Debug("system initialized");

		err = init_graphics(args, true);
		if (err) { log::Error("{}", err.message); goto system_exit; }
		log::Debug("graphics initialized");

		err = init_store(args);
		if (err) { log::Error("{}", err.message); goto graphics_exit; }
		log::Debug("store initialized");

		LogRuntimeInfo(true);
		LogGraphicsInfo();
		return err;

	graphics_exit:
		exit_graphics();
	system_exit:
		exit_system();
	logging_exit:
		exit_logging();
		return err;
	}

	bool Update() {
		update_store();
		return update_platform();
	}

	void Exit() {
		log::Debug("shutdown begin");
		exit_rml();
		log::Debug("rml shut down");
		exit_store();
		log::Debug("store shut down");
		exit_graphics();
		log::Debug("graphics shut down");
		exit_platform();
		log::Debug("platform shut down");
		exit_system();
		log::Debug("shutdown complete");
		exit_logging();
	}

	void ExitHeadless() {
		log::Debug("headless shutdown begin");
		exit_store();
		log::Debug("store shut down");
		exit_graphics();
		log::Debug("graphics shut down");
		exit_system();
		log::Debug("headless shutdown complete");
		exit_logging();
	}
} // namespace lf
