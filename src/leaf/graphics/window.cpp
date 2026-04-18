#include "window.hpp"
#include "detail/api.hpp"

namespace lf {
	handle<window> Window::Create(std::string_view title, dim2<i32> extent) {
		return Graphics.Window.create(title, extent);
	}

	void Window::Destroy(handle<window> wnd) {
		Graphics.Window.destroy(wnd);
	}

	void Window::Show(view<window> wnd) {
		Graphics.Window.show(wnd);
	}

	void Window::Hide(view<window> wnd) {
		Graphics.Window.hide(wnd);
	}

	dim2<i32> Window::GetSize(view<const window> wnd) {
		return Graphics.Window.get_size(wnd);
	}

	void Window::Resize(view<window> wnd, dim2<i32> extent) {
		Graphics.Window.resize(wnd, extent);
	}

	view<framebuffer> Window::BeginFrame(view<window> wnd) {
		return Graphics.Window.begin_frame(wnd);
	}

	void Window::EndFrame(view<window> wnd) {
		Graphics.Window.end_frame(wnd);
	}
}
