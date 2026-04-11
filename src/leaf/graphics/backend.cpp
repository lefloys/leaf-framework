#include "backend.hpp"

#include <cassert>

namespace lf {
	LF_API const GraphicsBackend* g_active_graphics_backend = nullptr;

	const GraphicsBackend& GraphicsBackend::GetActive() {
		auto* ptr = &g_active_graphics_backend;
		assert(*ptr && "No active GraphicsBackend set");
		return **ptr;
	}

	void GraphicsBackend::Switch(const GraphicsBackend* backend) {
		g_active_graphics_backend = backend;
	}

	bool GraphicsBackend::HasActive() {
		return g_active_graphics_backend != nullptr;
	}
}
