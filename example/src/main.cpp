#include <leaf/leaf.hpp>
#include <leaf/graphics/window.hpp>
#include <iostream>
#include <leaf/native_window.hpp>

int main(int argc, char* argv[]) {
	if (auto err = lf::LoadWindowBackend("leaf-framework-window-glfw.dll"); err) { std::cerr << err.message << '\n'; return 1; }
	if (auto err = lf::LoadGraphicsBackend("leaf-framework-graphics-vulkan.dll"); err) { std::cerr << err.message << '\n'; return 1; }

	if (auto err = lf::Init(argc, argv); err) {
		std::cerr << err.message << '\n';
		return 1;
	}
	lf::handle<lf::Window> wnd = lf::Window::Create("Hello, Leaf!", 800, 600);

	lf::Exit();
}