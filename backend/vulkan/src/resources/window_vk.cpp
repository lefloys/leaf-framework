#include "window_vk.hpp"

#include "leaf/core/exception.hpp"

namespace lf::detail::vk {
	template<>
	resource_pool<resource::window, WindowVK>& Pool<resource::window, WindowVK>() {
		return get_context().windows;
	}

	WindowVK::WindowVK(string_view title, dim2<i32> extent) {
		context& ctx = get_context();

		platform_window = create_platform_window(title, extent);

		result<vk_surface_handle> platform_surface = create_platform_vulkan_surface(
			reinterpret_cast<vk_instance_handle>(ctx.instance),
			platform_window
		);
		if (!platform_surface) {
			destroy_platform_window(platform_window);
			platform_window = nullptr;
			throw runtime_exception("failed to create Vulkan window surface");
		}

		surface = reinterpret_cast<VkSurfaceKHR>(*platform_surface);
	}

	WindowVK::~WindowVK() {
		context& ctx = get_context();

		if (surface && ctx.instance) {
			vkDestroySurfaceKHR(ctx.instance, surface, nullptr);
		}
		if (platform_window) {
			destroy_platform_window(platform_window);
		}

		surface = VK_NULL_HANDLE;
		platform_window = nullptr;
	}

	namespace Window {
		handle<window> Create(string_view title, dim2<i32> extent) {
			if (!has_context()) {
				throw runtime_exception("the Vulkan backend was used before lf::Init completed");
			}

			return Pool<resource::window, WindowVK>().create(title, extent);
		}

		void Destroy(handle<window> wnd) {
			Pool<resource::window, WindowVK>().destroy(wnd);
		}

		void Show(view<window> wnd) {
			show_platform_window(Unhandle<WindowVK>(wnd).platform_window);
		}

		void Hide(view<window> wnd) {
			hide_platform_window(Unhandle<WindowVK>(wnd).platform_window);
		}

		void Resize(view<window> wnd, dim2<i32> extent) {
			set_platform_window_extent(Unhandle<WindowVK>(wnd).platform_window, extent);
		}

		dim2<i32> GetSize(view<const window> wnd) {
			return get_platform_window_extent(Unhandle<WindowVK>(wnd).platform_window);
		}

		view<framebuffer> BeginFrame(view<window> wnd) {
			(void)wnd;
			throw runtime_exception("Window::BeginFrame is not implemented until the Vulkan swapchain path exists");
		}

		void EndFrame(view<window> wnd) {
			(void)wnd;
			throw runtime_exception("Window::EndFrame is not implemented until the Vulkan swapchain path exists");
		}
	}
}
