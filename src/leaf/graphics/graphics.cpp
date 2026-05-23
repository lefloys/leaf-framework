#include "graphics.hpp"

#define RUTILE_IMPL
#include <rutile.h>
#include <rt_ext_compute.h>
#include <rt_ext_glfw.h>
#include <rt_ext_swapchain.h>

#include <cstdlib>

namespace lf {
	bool is_logging_argument(string_view arg) {
		return arg == "-l" || arg == "--log" || arg == "--logging" || arg == "--logger";
	}

	bool is_validation_argument(string_view arg) {
		return arg == "-v" || arg == "--validation";
	}

	bool is_option_argument(string_view arg) {
		return arg.size() > 0 && arg[0] == '-';
	}

	bool is_graphics_argument(string_view arg) {
		return arg == "-g" || arg == "--graphics";
	}

	error init_graphics(span<string_view> args) {
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
			} else if (!is_option_argument(args[i])) {
				backend_name = args[i].data();
			}
		}

		const char* layers[2] = {};
		u32 layer_count = 0;
#ifndef NDEBUG
		if (enable_validation_layer) {
			layers[layer_count++] = "RT_VALIDATION";
		}
		if (enable_logging_layer) {
			layers[layer_count++] = "RT_LOGGING_LAYER";
		}
#else
		if (enable_logging_layer) {
			return error(generic_errc::unknown, "Rutile logging layer is not available in release builds");
		}
#endif
		if (rtLoad(backend_name, layers, layer_count) != RT_SUCCESS) {
			return error(generic_errc::unknown, rtErrorMessage());
		}
		if (!rtLoad_RT_EXT_SWAPCHAIN() || !rtLoad_RT_EXT_GLFW()) {
			rtUnload();
			return error(generic_errc::unknown, "required Rutile window extensions are not available");
		}

		const char* features[] = { RT_FEATURE_PRESENTATION };
		rtInit(features, 1);
		if (rtError() != RT_SUCCESS) {
			return error(generic_errc::unknown, "rtInit failed");
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
