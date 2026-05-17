#define RUTILE_IMPL
#include <rt_ext_compute.h>
#include <rt_ext_glfw.h>
#include <rt_ext_swapchain.h>
#include <rutile.h>

#include "leaf.hpp"

#include "leaf/scene/rml_backend.hpp"
#include "leaf/system/system.hpp"
#include "leaf/window/backend.hpp"

#include <GLFW/glfw3.h>

#include <ranges>

namespace lf {
	lf::vector<lf::error (*)(lf::span<lf::string_view>)> init_funcs;
	lf::vector<void (*)()> exit_funcs;

	static bool glfw_initialized = false;

	bool IsLoggingArgument(string_view arg) {
		return arg == "-l" || arg == "--log" || arg == "--logging" || arg == "--logger";
	}

	bool IsOptionArgument(string_view arg) {
		return arg.size() > 0 && arg[0] == '-';
	}

	bool IsGraphicsArgument(string_view arg) {
		return arg == "-g" || arg == "--graphics";
	}

	void RegisterLifetime(lf::error (*init_func)(lf::span<lf::string_view>), void (*exit_func)()) {
		init_funcs.push_back(init_func);
		exit_funcs.push_back(exit_func);
	}

	error Init(span<string_view> args) {
		InitSystem();

		if (!glfwInit()) {
			return error(generic_errc::unknown, "glfwInit failed");
		}
		glfw_initialized = true;

		const char* backend_name = "rt-vulkan";
		bool enable_logging_layer = false;
		for (usize i = 1; i < args.size(); ++i) {
			if (IsLoggingArgument(args[i])) {
				enable_logging_layer = true;
			} else if (IsGraphicsArgument(args[i]) && i + 1 < args.size()) {
				backend_name = args[++i].data();
			} else if (!IsOptionArgument(args[i])) {
				backend_name = args[i].data();
			}
		}

		const char* layers[] = { "RT_VALIDATION", "RT_LOGGING_LAYER" };
		const u32 layer_count = enable_logging_layer ? 2u : 1u;
		const enum rt_error load_result = rtLoad(backend_name, layers, layer_count);
		if (load_result != RT_SUCCESS) {
			glfwTerminate();
			glfw_initialized = false;
			return error(generic_errc::unknown, "rtLoad failed");
		}
		if (!rtLoad_RT_EXT_SWAPCHAIN() || !rtLoad_RT_EXT_GLFW()) {
			rtUnload();
			glfwTerminate();
			glfw_initialized = false;
			return error(generic_errc::unknown, "required Rutile window extensions are not available");
		}
		rtLoad_RT_EXT_COMPUTE();

		const char* features[] = { RT_FEATURE_PRESENTATION };
		rtInit(features, 1);
		const enum rt_error init_result = rtError();
		if (init_result != RT_SUCCESS) {
			string message = rtErrorMessage() ? rtErrorMessage() : "rtInit failed";
			rtClearError();
			rtUnload();
			glfwTerminate();
			glfw_initialized = false;
			return error(generic_errc::unknown, message);
		}

		error rml_err = initialize_rml_runtime();
		if (rml_err) {
			rtExit();
			rtUnload();
			glfwTerminate();
			glfw_initialized = false;
			return rml_err;
		}

		error err = error::no_error;
		for (auto& func : init_funcs) {
			if (!func) {
				continue;
			}
			err = func(args);
			if (err) {
				shutdown_rml_runtime();
				return err;
			}
		}

		return err;
	}

	bool Update() {
		return update_platform();
	}

	void Exit() {
		for (auto& func : std::ranges::reverse_view(exit_funcs)) {
			if (!func) {
				continue;
			}
			func();
		}
		shutdown_rml_runtime();
		if (rtLoaded()) {
			rtExit();
		}
		rtUnload();
		if (glfw_initialized) {
			glfwTerminate();
			glfw_initialized = false;
		}
	}
} // namespace lf
