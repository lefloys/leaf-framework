#include "graphics.hpp"

#include "leaf/core/logging.hpp"
#include "leaf/config.hpp"

#include <rutile.h>
#include <rt_ext_glfw.h>
#include <rt_ext_swapchain.h>

#include <cstdlib>



namespace rt {
	// @GPT : why anonymous namespace. why is this "DefaultGraphicsAPI" hardcoded like THIS
	namespace {
		constexpr string_view DefaultGraphicsAPI = "rt-opengl";
		// @GPT : CLI11 ??
		error parse_graphics_backend(span<string_view> args, string_view& backend) {
			for (size_t i = 0; i < args.size(); ++i) {
				string_view arg = args[i];
				if (arg == "-g" || arg == "--graphics") {
					if (i + 1 == args.size() || args[i + 1].empty() || args[i + 1].front() == '-') {
						return error(generic_errc::input_error, "missing value for -g/--graphics");
					}
					backend = args[++i];
					continue;
				}

				constexpr string_view LongPrefix = "--graphics=";
				if (arg.starts_with(LongPrefix)) {
					string_view value = arg.substr(LongPrefix.size());
					if (value.empty()) {
						return error(generic_errc::input_error, "missing value for --graphics");
					}
					backend = value;
				}
			}
			return error::no_error;
		}
	}

	void rutile_log_output(const char* message, void*) {
		if (!message || !message[0]) {
			return;
		}
		// @GPT : most vexing parse. dont do this shit. use {}
		string_view text(message);
		while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
			text.remove_suffix(1);
		}
		if (!text.empty()) {
			lf::log::Debug("{}", text);
		}
	}
	static error rutile_error(enum rt_error err, string_view context) {
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
		case RT_FEATURE_NOT_SUPPORTED: /******/ code = graphics_errc::unsupported_feature; /****/ break;
		default: UNREACHABLE(); break;
		}
		const char* message = rtErrorMessage();
		if (!message || !message[0]) {
			message = "no backend error message";
		}
		return error(code, lf::format("{} failed: {}", context, message));
	}
	// @GPT : Why is this in graphics.cpp
	error rutile_error() {
		return rutile_error(rtError(), "Rutile call");
	}



	error init_graphics(span<string_view> args, bool headless) {
		// @GPT : Why arent you giving any information about what backend you are starting and the version of it. 
		lf::log::Info("[leaf] Starting graphics...");
		string_view graphics_api = DefaultGraphicsAPI;
		// @GPT : Why are you using some useless thing. why not CLI11 ??
		if (error err = parse_graphics_backend(args, graphics_api)) {
			return err;
		}
		lf::log::Debug("[leaf] Graphics init for '{}'", graphics_api);
		// @GPT : this is not the way i intended. it was suppoed to be "key" (for example rt-opengl) and "value" eg "-v 4.6". thats what rutile is supposed to support. and any backend can react to any key. ffs 
		rtSettingAdd("opengl.version", "4.6");
		if (auto err = rtLoad(graphics_api.data(), nullptr, 0)) {
			lf::log::Error("[leaf] Failed to load graphics backend '{}'", graphics_api);
			return error(generic_errc::unknown, "rtLoad failed");
		}
		lf::log::Debug("[leaf] Loaded graphics backend '{}'", graphics_api);
		rtSetOutput(rutile_log_output, nullptr);
		if (headless) {
			lf::log::Debug("[leaf] Initializing '{}' without features.", graphics_api);
			rtInit(nullptr, 0);
		} else {
			const char* features[] = { RT_FEATURE_PRESENTATION };
			lf::log::Debug("[leaf] Initializing '{}' with presentation feature.", graphics_api);
			rtInit(features, 1);
		}

		error err = rutile_error(rtError(), "rtInit");
		if (err) {
			lf::log::Error("Graphics initialization failed: {}", err.message);
			rtUnload();
			return err;
		}
		return error::no_error;
	}

	error init_graphics_extensions(bool headless) {
		if (!headless) {
			if (auto err = rtLoad_RT_EXT_SWAPCHAIN(); err != RT_SUCCESS) {
				return rutile_error(err, "rtLoad_RT_EXT_SWAPCHAIN");
			}
			if (auto err = rtLoad_RT_EXT_GLFW(); err != RT_SUCCESS) {
				return rutile_error(err, "rtLoad_RT_EXT_GLFW");
			}
			lf::log::Trace("[leaf] Loaded Rutile swapchain and GLFW presentation extensions");
		}
		return error::no_error;
	}

	void exit_graphics() {
		lf::log::Debug("Shutting down graphics: {}", GraphicsBackendName());
		rtExit();
		rtUnload();
	}

	bool graphics_available() {
		// @GPT : Random bool ???
		// The Rutile dispatch table is populated by rtLoad; before that the
		// function pointers are null and any rt* call would crash.
		return rt_rtQueueQuery != nullptr;
	}

	string_view GraphicsBackendName() { return rtGetName(); }
}

