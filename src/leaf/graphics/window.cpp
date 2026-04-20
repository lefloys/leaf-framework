#include "window.hpp"
#include "framebuffer.hpp"
#include "detail/api.hpp"

namespace lf {
	handle<window> Window::Create(string_view title, dim2<i32> extent) {
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
		Graphics.Window.acquire_image(wnd);
		return Graphics.Window.get_framebuffer(wnd);
	}

	void Window::EndFrame(view<window> wnd) {
		Graphics.Window.present(wnd);
	}

	bool Window::ShouldClose(view<const window> wnd) {
		return Graphics.Window.should_close(wnd);
	}
}
