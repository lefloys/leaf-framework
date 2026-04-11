#include "window.hpp"
#include "window.hpp"
#include "../leaf-graphics-vulkan.hpp"

#include <leaf/leaf.hpp>

#include <cassert>

namespace lf::vk {
	WindowVK::WindowVK(string_view title, dim2<u32> extent) {
		const auto* api = GetHostAPI();
		assert(api && api->window_backend && "Host API / window backend not initialized");
		native = api->window_backend->create_window(title, extent);
	}
	WindowVK::~WindowVK() {
		const auto* api = GetHostAPI();
		assert(api && api->window_backend && "Host API / window backend not initialized");
		api->window_backend->destroy_window(native);
	}
	handle<Window> WindowVK::create(string_view title, u32 width, u32 height) {
		return resource_manager.create(title, dim2<u32>{ width, height });
	}
	Window::Backend WindowVK::get_backend() {
		Window::Backend backend;
		backend.create = create;
		return backend;
	}
}