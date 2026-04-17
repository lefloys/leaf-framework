#include <leaf/leaf.hpp>
#include <leaf/graphics/window.hpp>
#include <leaf/graphics/command_buffer.hpp>
#include <iostream>


int main(int argc, char* argv[]) {
	lf::handle<lf::window> wnd = lf::Window::Create("Hello Leaf!", { 1280, 720 });

	lf::Window::Destroy(wnd);
}