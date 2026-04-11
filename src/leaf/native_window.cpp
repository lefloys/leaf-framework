#include "native_window.hpp"
#include "native_window.hpp"

#include <cassert>

namespace lf {
	LF_API const WindowBackend* g_active_window_backend = nullptr;

	const WindowBackend& WindowBackend::GetActive() {
		auto* ptr = &g_active_window_backend;
		assert(*ptr && "No active WindowBackend set");
		return **ptr;
	}

	void WindowBackend::Switch(const WindowBackend* backend) {
		g_active_window_backend = backend;
	}

	bool WindowBackend::HasActive() { return g_active_window_backend != nullptr; }

	error init_window_backend() { return WindowBackend::GetActive().init(); }
	void exit_window_backend() { return WindowBackend::GetActive().exit(); }
	NativeWindow* create_window(std::string_view title, dim2<u32> extent) { return WindowBackend::GetActive().create_window(title, extent); }
	void destroy_window(NativeWindow* native) { return WindowBackend::GetActive().destroy_window(native); }
}
