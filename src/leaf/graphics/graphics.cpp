#include "graphics.hpp"

#define RUTILE_IMPL
#include <rutile.h>
#include <rt_ext_compute.h>
#include <rt_ext_glfw.h>
#include <rt_ext_swapchain.h>


namespace lf {
	bool is_logging_argument(string_view arg) {
		return arg == "-l" || arg == "--log" || arg == "--logging" || arg == "--logger";
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
		for (u64 i = 1; i < args.size(); ++i) {
			if (is_logging_argument(args[i])) {
				enable_logging_layer = true;
			} else if (is_graphics_argument(args[i]) && i + 1 < args.size()) {
				backend_name = args[++i].data();
			} else if (!is_option_argument(args[i])) {
				backend_name = args[i].data();
			}
		}

		const char* layers[] = { "RT_VALIDATION", "RT_LOGGING_LAYER" };
		const u32 layer_count = enable_logging_layer ? 2u : 1u;
		if (rtLoad(backend_name, layers, layer_count) != RT_SUCCESS) {
			return error(generic_errc::unknown, "rtLoad failed");
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

}
