

#include "leaf.hpp"

#include "leaf/core/error.hpp"
#include "leaf/core/messages.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"
#include "leaf/store/lifecycle.hpp"
#include "leaf/graphics/window.hpp"
#include "leaf/application/rml_backend.hpp"
#include "leaf/system/system.hpp"
#include "leaf/platform/platform.hpp"
#include "leaf/graphics/graphics.hpp"

namespace lf {
	error Init(span<string_view> args) {
		LF_LOG_INFO("leaf.lifecycle", "init begin");
		error err;

		err = init_system(args);
		if (err) { LF_LOG_ERROR("leaf.lifecycle", err.message); goto system_exit; }
		LF_LOG_DEBUG("leaf.lifecycle", "system initialized");

		err = init_platform(args);
		if (err) { LF_LOG_ERROR("leaf.lifecycle", err.message); goto platform_exit; }
		LF_LOG_DEBUG("leaf.lifecycle", "platform initialized");

		err = init_graphics(args);
		if (err) { LF_LOG_ERROR("leaf.lifecycle", err.message); goto graphics_exit; }
		LF_LOG_DEBUG("leaf.lifecycle", "graphics initialized");

		err = init_store(args);
		if (err) { LF_LOG_ERROR("leaf.lifecycle", err.message); goto store_exit; }
		LF_LOG_DEBUG("leaf.lifecycle", "store initialized");

		err = init_rml(args); 
		if (err) { LF_LOG_ERROR("leaf.lifecycle", err.message); goto rml_exit; }
		LF_LOG_INFO("leaf.lifecycle", "init complete");

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

	bool Update() {
		update_store();
		return update_platform();
	}

	void Exit() {
		LF_LOG_INFO("leaf.lifecycle", "shutdown begin");
		exit_rml();
		LF_LOG_DEBUG("leaf.lifecycle", "rml shut down");
		exit_store();
		LF_LOG_DEBUG("leaf.lifecycle", "store shut down");
		exit_graphics();
		LF_LOG_DEBUG("leaf.lifecycle", "graphics shut down");
		exit_platform();
		LF_LOG_DEBUG("leaf.lifecycle", "platform shut down");
		exit_system();
		LF_LOG_INFO("leaf.lifecycle", "shutdown complete");
		log::Shutdown();
	}
} // namespace lf
