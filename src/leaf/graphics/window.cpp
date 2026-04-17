#include "window.hpp"

namespace lf {
	handle<window> Window::Create(std::string_view title, dim2<u32> extent) {
		// call into the backend to allocate a window and build it
		return {};
	}
	void Window::Destroy(handle<window> wnd) {
		// call into the backend to release the window and reset it
	}
}