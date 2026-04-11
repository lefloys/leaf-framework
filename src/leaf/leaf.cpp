
#include "leaf.hpp"

#include "leaf/graphics/backend.hpp"
#include "leaf/native_window.hpp"

#include <iostream>
#include <string>
#include <vector>

#include <Windows.h>

namespace lf {
	namespace {
		using module_handle = HMODULE;
		HostAPI g_host_api{};

		std::vector<module_handle>& LoadedModules() {
			static std::vector<module_handle> modules;
			return modules;
		}

		error LoadModule(std::string_view module_path, module_handle& out_module) {
			const std::string path(module_path);
			out_module = LoadLibraryA(path.c_str());
			if (!out_module) {
				return error(generic_errc::unknown, "Failed to load backend module: " + path);
			}

			return error::no_error;
		}
	}

	error LoadWindowBackend(std::string_view module_path) {
		module_handle module = nullptr;
		if (auto err = LoadModule(module_path, module); err) {
			return err;
		}

		using get_window_backend_t = const WindowBackend* (*)();
		auto* get_window_backend = reinterpret_cast<get_window_backend_t>(GetProcAddress(module, "lf_get_window_backend"));
		if (!get_window_backend) {
			FreeLibrary(module);
			return error(generic_errc::unknown, "Module does not expose a window backend entrypoint: " + std::string(module_path));
		}

		WindowBackend::Switch(get_window_backend());
		g_host_api.window_backend = get_window_backend();
		LoadedModules().push_back(module);
		return error::no_error;
	}

	error LoadGraphicsBackend(std::string_view module_path) {
		module_handle module = nullptr;
		if (auto err = LoadModule(module_path, module); err) {
			return err;
		}

		using get_graphics_backend_t = const GraphicsBackend* (*)();
		auto* get_graphics_backend = reinterpret_cast<get_graphics_backend_t>(GetProcAddress(module, "lf_get_graphics_backend"));
		if (!get_graphics_backend) {
			FreeLibrary(module);
			return error(generic_errc::unknown, "Module does not expose a graphics backend entrypoint: " + std::string(module_path));
		}

		GraphicsBackend::Switch(get_graphics_backend());

		using set_host_api_t = void(*)(const HostAPI* api);
		auto* set_host_api = reinterpret_cast<set_host_api_t>(GetProcAddress(module, "lf_set_host_api"));
		if (set_host_api) {
			set_host_api(&g_host_api);
		}

		LoadedModules().push_back(module);
		return error::no_error;
	}

	const HostAPI* GetHostAPI() {
		return &g_host_api;
	}

	error Init(i32 argc, char* argv[]) {
		std::cout << "Hello leaf!\n";
		if (!WindowBackend::HasActive()) {
			return error(generic_errc::unknown, "No active window backend selected");
		}

		if (!GraphicsBackend::HasActive()) {
			return error(generic_errc::unknown, "No active graphics backend selected");
		}

		if (auto err = WindowBackend::GetActive().init(); err) {
			return err.add_context("Failed to initialize window backend");
		}

		if (auto err = GraphicsBackend::GetActive().init(argc, argv); err) {
			WindowBackend::GetActive().exit();
			return err.add_context("Failed to initialize graphics backend");
		}

		return error::no_error;
	}
	void Exit() {
		if (GraphicsBackend::HasActive()) {
			GraphicsBackend::GetActive().exit();
		}
		if (WindowBackend::HasActive()) {
			WindowBackend::GetActive().exit();
		}

		for (auto module : LoadedModules()) {
			if (module) {
				FreeLibrary(module);
			}
		}
		LoadedModules().clear();
		g_host_api = {};

	}
}