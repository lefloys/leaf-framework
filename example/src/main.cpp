#include <leaf/leaf.hpp>
#include <leaf/graphics/backends/vulkan.hpp>
#include <leaf/graphics/window.hpp>
#include <leaf/platform/backends/glfw.hpp>

#include <chrono>
#include <iostream>
#include <thread>


int main(int argc, char* argv[]) {
	lf::SetPlatformAPI(lf::CreateGLFWPlatformAPI());
	lf::SetGraphicsAPI(lf::CreateVulkanGraphicsAPI());

	if (lf::error err = lf::Init(argc, argv)) {
		std::cerr << err.message << '\n';
		return err.code.value();
	}

	lf::handle<lf::window> wnd = lf::Window::Create("Hello Leaf!", { 1280, 720 });
	lf::Window::Show(wnd);

	lf::dim2<i32> size = lf::Window::GetSize(wnd);
	std::cout << "Window size: " << size.width << "x" << size.height << '\n';
	std::this_thread::sleep_for(std::chrono::seconds(1));

	lf::Exit();
}
