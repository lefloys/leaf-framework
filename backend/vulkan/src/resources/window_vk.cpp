#include "window_vk.hpp"

#include "leaf/core/exception.hpp"
#include "leaf/platform/api.hpp"

WindowVK::WindowVK(lf::string_view title, lf::dim2<i32> extent) {
	context& ctx = get_context();

	if (!lf::Platform.create_platform_window || !lf::Platform.create_platform_vulkan_surface || !lf::Platform.destroy_platform_window) {
		throw lf::runtime_exception("the active platform backend does not provide the required window Vulkan hooks");
	}

	platform_window = lf::Platform.create_platform_window(title, extent);

	lf::result<lf::VkSurface> platform_surface = lf::Platform.create_platform_vulkan_surface(ctx.vk_instance, platform_window);
	if (!platform_surface) {
		lf::Platform.destroy_platform_window(platform_window);
		platform_window = nullptr;
		throw lf::runtime_exception("failed to create Vulkan window surface");
	}

	surface = reinterpret_cast<VkSurfaceKHR>(*platform_surface);
}

WindowVK::~WindowVK() {
	context& ctx = get_context();

	vkDestroySurfaceKHR(ctx.vk_instance, surface, nullptr);
	if (platform_window && lf::Platform.destroy_platform_window) {
		lf::Platform.destroy_platform_window(platform_window);
	}

	surface = VK_NULL_HANDLE;
	platform_window = nullptr;
}

namespace Window {
	lf::handle<lf::window> Create(lf::string_view title, lf::dim2<i32> extent) {
		if (!has_context()) {
			throw lf::runtime_exception("the Vulkan backend was used before lf::Init completed");
		}

		return get_context().windows.create(title, extent);
	}

	void Destroy(lf::handle<lf::window> wnd) {
		get_context().windows.destroy(wnd);
	}

	void Show(lf::view<lf::window> wnd) {
		if (lf::Platform.show_platform_window) {
			lf::Platform.show_platform_window(get_context().windows.get(wnd).platform_window);
		}
	}

	void Hide(lf::view<lf::window> wnd) {
		if (lf::Platform.hide_platform_window) {
			lf::Platform.hide_platform_window(get_context().windows.get(wnd).platform_window);
		}
	}

	void Resize(lf::view<lf::window> wnd, lf::dim2<i32> extent) {
		if (lf::Platform.set_platform_window_extent) {
			lf::Platform.set_platform_window_extent(get_context().windows.get(wnd).platform_window, extent);
		}
	}

	lf::dim2<i32> GetSize(lf::view<const lf::window> wnd) {
		return lf::Platform.get_platform_window_extent
			? lf::Platform.get_platform_window_extent(get_context().windows.get(wnd).platform_window)
			: lf::dim2<i32>{};
	}

	lf::view<lf::framebuffer> BeginFrame(lf::view<lf::window> wnd) {
		(void)wnd;
		throw lf::runtime_exception("Window::BeginFrame is not implemented until the Vulkan swapchain path exists");
	}

	void EndFrame(lf::view<lf::window> wnd) {
		(void)wnd;
		throw lf::runtime_exception("Window::EndFrame is not implemented until the Vulkan swapchain path exists");
	}
}
