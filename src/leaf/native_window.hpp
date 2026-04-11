#pragma once

#pragma once

#include "leaf/math/dim.hpp"
#include "leaf/core/error.hpp"
#include "leaf/export.hpp"


#include <string>

namespace lf {
	struct NativeWindow;
		extern LF_API const struct WindowBackend* g_active_window_backend;

	LF_API error init_window_backend();
	LF_API void exit_window_backend();
	LF_API NativeWindow* create_window(std::string_view title, dim2<u32> extent);
	LF_API void destroy_window(NativeWindow* native);

	struct LF_API WindowBackend {
		static const WindowBackend& GetActive();
		static void Switch(const WindowBackend* backend);
		static bool HasActive();

		decltype(init_window_backend)* init = nullptr;
		decltype(exit_window_backend)* exit = nullptr;
		decltype(create_window)* create_window = nullptr;
		decltype(destroy_window)* destroy_window = nullptr;
	};
}