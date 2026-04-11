#include "window.hpp"

#include "leaf/graphics/backend.hpp"

lf::handle<lf::Window> lf::Window::Create(std::string_view title, u32 width, u32 height) {
	return GraphicsBackend::GetActive().window_backend.create(title, width, height);
}