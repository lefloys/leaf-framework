#include <leaf/leaf.hpp>
#include <leaf/core/iostream.hpp>
#include <leaf/graphics/backends/vulkan.hpp>
#include <leaf/graphics/window.hpp>
#include <leaf/platform/backends/glfw.hpp>

int main() {
	lf::SetPlatformAPI(lf::CreateGLFWPlatformAPI());
	lf::SetGraphicsAPI(lf::CreateVulkanGraphicsAPI());

	if (lf::error err = lf::Init()) {
		lf::cerr << err.message << '\n';
		return err.code.value();
	}

	lf::handle<lf::window> wnd = lf::Window::Create("Hello Leaf!", { 1280, 720 });
	lf::Window::Show(wnd);

	lf::dim2<i32> size = lf::Window::GetSize(wnd);
	lf::cout << "Window size: " << size.width << "x" << size.height << '\n';
	while (true) {
		lf::Window::BeginFrame(wnd);
		lf::Window::EndFrame(wnd);
	}

	lf::Window::Destroy(wnd);

	lf::Exit();
}
