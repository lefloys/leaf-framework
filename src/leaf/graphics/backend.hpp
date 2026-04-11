#pragma once
#pragma once



#include "window.hpp"
#include "leaf/core/error.hpp"
#include "leaf/export.hpp"

#include <cassert>

namespace lf {
	extern LF_API const struct GraphicsBackend* g_active_graphics_backend;

	struct LF_API GraphicsBackend {
		
		Window::Backend window_backend;

		error(*init)(int argc, char* argv[]) = nullptr;
		void(*exit)() = nullptr;

		static const GraphicsBackend& GetActive();
		static void Switch(const GraphicsBackend* backend);
		static bool HasActive();
	};
}
