#include "graphics.hpp"

#include <leaf/logging/logging.hpp>

#define RUTILE_IMPL
#include <rutile.h>
#include <rt_ext_compute.h>
#include <rt_ext_glfw.h>
#include <rt_ext_swapchain.h>

#include <cstdlib>



namespace lf {
	void rutile_log_output(const char* message, void*) {
		if (!message || !message[0]) {
			return;
		}

		string_view text(message);
		while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
			text.remove_suffix(1);
		}
		if (!text.empty()) {
			log::Debug("{}", text);
		}
	}
	error to_error(rt_error err) {
		if (err == RT_SUCCESS) { return error::no_error; }

		error_code code;
		switch (err) {
		case RT_OUT_OF_HOST_MEMORY: /*********/ code = graphics_errc::out_of_host_memory; /*****/ break;
		case RT_OUT_OF_DEVICE_MEMORY:/********/ code = graphics_errc::out_of_device_memory; /***/ break;
		case RT_IMPROPER_USAGE: /*************/ code = graphics_errc::invalid_argument; /*******/ break;
		case RT_PLATFORM_FAILURE:/************/ code = graphics_errc::platform_failure; /*******/ break;
		case RT_DEVICE_LOST: /****************/ code = graphics_errc::device_lost; /************/ break;
		case RT_ALREADY_INITIALIZED: /********/ code = graphics_errc::already_exists; /*********/ break;
		case RT_NO_BACKEND: /*****************/ code = graphics_errc::not_supported; /**********/ break;
		case RT_UNSUPPORTED_PLATFORM: /*******/ code = graphics_errc::not_supported; /**********/ break;
		case RT_UNSUPPORTED_FEATURE: /********/ code = graphics_errc::unsupported_feature; /****/ break;
		case RT_INITIALIZATION_FAILED: /******/ code = graphics_errc::initialization; /*********/ break;
		case RT_LAYER_NOT_PRESENT: /**********/ code = graphics_errc::layer_not_present; /******/ break;
		case RT_EXTENSION_NOT_PRESENT: /******/ code = graphics_errc::extension_not_present; /**/ break;
		case RT_INCOMPATIBLE_DRIVER: /********/ code = graphics_errc::incompatible_driver; /****/ break;
		case RT_SHADER_COMPILATION_FAILED: /**/ code = graphics_errc::shader_compilation; /*****/ break;
		case RT_SHADER_LINK_FAILED: /*********/ code = graphics_errc::shader_link_failed; /*****/ break;
		default: UNREACHABLE(); break;
		}
		return error(code, format("rtInit failed: {}", rtErrorMessage()));
	}



	error init_graphics(span<string_view> args, bool headless) {
		log::Info("[leaf] Starting graphics...");
		constexpr string_view DefaultGraphicsAPI = "rt-vulkan"
		log::Debug("[leaf] Graphics init for '{}'", DefaultGraphicsAPI);
		if (auto err = rtLoad(DefaultGraphicsAPI.data(), nullptr, 0)) {
			log::Error("[leaf] Failed to load graphics backend '{}'", DefaultGraphicsAPI);
			return error(generic_errc::unknown, "rtLoad failed");
		}
		log::Debug("[leaf] Loaded graphics backend '{}'", DefaultGraphicsAPI);
		rtSetOutput(rutile_log_output, nullptr);
		if (!rtLoad_RT_EXT_COMPUTE()) {
			rtUnload();
			return error(generic_errc::unknown, "required Rutile compute extension is not available");
		}
		log::Trace("[leaf] Loaded Rutile compute extension");

		if (!headless) {
			if (!rtLoad_RT_EXT_SWAPCHAIN() || !rtLoad_RT_EXT_GLFW()) {
				rtUnload();
				return error(generic_errc::unknown, "required Rutile presentation extensions are not available");
			}
			log::Trace("[leaf] Loaded Rutile swapchain and GLFW presentation extensions");
		}

		if (headless) {
			log::Debug("[leaf] Initializing '{}' without features.", DefaultGraphicsAPI);
			rtInit(nullptr, 0);
		} else {
			const char* features[] = { RT_FEATURE_PRESENTATION };
			log::Debug("[leaf] Initializing '{}' with presentation feature.", DefaultGraphicsAPI);
			rtInit(features, 1);
		}

		error err = to_error();
		if (err) {
			log::Error("Graphics initialization failed: {}", err.message);
			rtUnload();
			return err;
		}
		return error::no_error;
	}

	void exit_graphics() {
		log::Debug("Shutting down graphics: {}", GraphicsBackendName());
		rtExit();
		rtUnload();
	}

	string_view GraphicsBackendName() { return rtGetName(); }
}
