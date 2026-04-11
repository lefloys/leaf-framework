#include <leaf/leaf.hpp>
#include <leaf/graphics/window.hpp>
#include <iostream>

int main(int argc, char* argv[]) {

	if (auto err = lf::Init(argc, argv); err) {
		std::cerr << err.message << '\n';
		return 1;
	}
	lf::handle<lf::Window> wnd = lf::Window::Create("Hello, Leaf!", 800, 600);
	lf::Window::Destroy(wnd);

	lf::Exit();
}