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

	bool is_logging_argument(string_view arg) {
		return arg == "-l" || arg == "--log" || arg == "--logging" || arg == "--logger";
	}

	bool is_validation_argument(string_view arg) {
		return arg == "-v" || arg == "--validation";
	}

	bool is_graphics_argument(string_view arg) {
		return arg == "-g" || arg == "--graphics";
	}

	struct RutileLoadOptions {
		const char* backend_name = "rt-vulkan";
		const char* layers[2] = {};
		u32 layer_count = 0;
	};

	error rutile_init_error() {
		enum rt_error init_error = rtError();
		if (init_error == RT_SUCCESS) {
			return error::no_error;
		}

		const char* message = rtErrorMessage();
		if (message && message[0]) {
			return error(generic_errc::unknown, format("rtInit failed: {}", message));
		}
		string error_name = "unknown error";
		switch (init_error) {
		case RT_OUT_OF_HOST_MEMORY: error_name = "out of host memory"; break;
		case RT_OUT_OF_DEVICE_MEMORY: error_name = "out of device memory"; break;
		case RT_IMPROPER_USAGE: error_name = "improper usage"; break;
		case RT_PLATFORM_FAILURE: error_name = "platform failure"; break;
		case RT_DEVICE_LOST: error_name = "device lost"; break;
		case RT_ALREADY_INITIALIZED: error_name = "already initialized"; break;
		case RT_UNSUPPORTED_PLATFORM: error_name = "unsupported platform"; break;
		case RT_NO_BACKEND: error_name = "no backend"; break;
		case RT_UNSUPPORTED_FEATURE: error_name = "unsupported feature"; break;
		case RT_INITIALIZATION_FAILED: error_name = "initialization failed"; break;
		case RT_LAYER_NOT_PRESENT: error_name = "layer not present"; break;
		case RT_EXTENSION_NOT_PRESENT: error_name = "extension not present"; break;
		case RT_INCOMPATIBLE_DRIVER: error_name = "incompatible driver"; break;
		case RT_SHADER_COMPILATION_FAILED: error_name = "shader compilation failed"; break;
		case RT_SHADER_LINK_FAILED: error_name = "shader link failed"; break;
		default: break;
		}
		return error(generic_errc::unknown, format("rtInit failed: {} ({})", error_name, static_cast<int>(init_error)));
	}

	report<RutileLoadOptions> parse_rutile_load_options(span<string_view> args) {
		RutileLoadOptions options;
		const char* backend_name = "rt-vulkan";
		bool enable_logging_layer = false;
		bool enable_validation_layer = std::getenv("LEAF_VALIDATION") != nullptr;
		for (u64 i = 1; i < args.size(); ++i) {
			if (is_logging_argument(args[i])) {
				enable_logging_layer = true;
			} else if (is_validation_argument(args[i])) {
				enable_validation_layer = true;
			} else if (is_graphics_argument(args[i]) && i + 1 < args.size()) {
				backend_name = args[++i].data();
			}
		}
		options.backend_name = backend_name;

#ifndef NDEBUG
		if (enable_validation_layer) {
			options.layers[options.layer_count++] = "RT_VALIDATION";
		}
		if (enable_logging_layer) {
			options.layers[options.layer_count++] = "RT_LOGGING_LAYER";
		}
#else
		if (enable_logging_layer) {
			return unexpected(error(generic_errc::unknown, "Rutile logging layer is not available in release builds"));
		}
#endif
		return options;
	}

	error init_graphics(span<string_view> args, bool headless) {
		report<RutileLoadOptions> options = parse_rutile_load_options(args);
		if (!options) {
			return options.error();
		}

		if (rtLoad(options->backend_name, options->layers, options->layer_count) != RT_SUCCESS) {
			return error(generic_errc::unknown, "rtLoad failed");
		}
		rtSetOutput(rutile_log_output, nullptr);
		if (!rtLoad_RT_EXT_COMPUTE()) {
			rtUnload();
			return error(generic_errc::unknown, "required Rutile compute extension is not available");
		}

		if (!headless) {
			if (!rtLoad_RT_EXT_SWAPCHAIN() || !rtLoad_RT_EXT_GLFW()) {
				rtUnload();
				return error(generic_errc::unknown, "required Rutile presentation extensions are not available");
			}
		}

		if (headless) {
			rtInit(nullptr, 0);
		} else {
			const char* features[] = { RT_FEATURE_PRESENTATION };
			rtInit(features, 1);
		}

		error err = rutile_init_error();
		if (err) {
			rtUnload();
			return err;
		}
		return error::no_error;
	}

	void exit_graphics() {
		rtExit();
		rtUnload();
	}

	string_view graphics_backend_name() {
		if (!rt_rtGetName) {
			return "unknown";
		}
		const char* name = rtGetName();
		return name ? string_view(name) : string_view("unknown");
	}

}
