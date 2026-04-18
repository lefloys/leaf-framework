#include "leaf-graphics-vulkan.hpp"

#include "resources/window_vk.hpp"
#include "vulkan_context.hpp"

namespace lf::detail::vk {
	error Init() {
		if (has_context()) {
			return error::no_error;
		}

		create_context();
		context& ctx = get_context();

		if (!platform_vulkan_supported()) {
			destroy_context();
			return error(generic_errc::unknown, "the active platform backend reports that Vulkan is not supported");
		}

		u32 extension_count = 0;
		const char** extensions = get_platform_vulkan_instance_extensions(extension_count);
		if (!extensions || extension_count == 0) {
			destroy_context();
			return error(generic_errc::unknown, "failed to query required Vulkan instance extensions from the platform backend");
		}

		VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		app_info.pApplicationName = "leaf-framework";
		app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
		app_info.pEngineName = "leaf";
		app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
		app_info.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		create_info.pApplicationInfo = &app_info;
		create_info.enabledExtensionCount = extension_count;
		create_info.ppEnabledExtensionNames = extensions;

		if (VkResult result = vkCreateInstance(&create_info, nullptr, &ctx.instance); result != VK_SUCCESS) {
			destroy_context();
			return error(generic_errc::unknown, "failed to create Vulkan instance");
		}

		return error::no_error;
	}

	void Exit() {
		if (!has_context()) {
			return;
		}

		destroy_context();
	}
}

namespace lf {
	GraphicsAPI CreateVulkanGraphicsAPI() {
		GraphicsAPI api{};
		api.init = &detail::vk::Init;
		api.exit = &detail::vk::Exit;
		api.Window.create = &detail::vk::Window::Create;
		api.Window.destroy = &detail::vk::Window::Destroy;
		api.Window.show = &detail::vk::Window::Show;
		api.Window.hide = &detail::vk::Window::Hide;
		api.Window.resize = &detail::vk::Window::Resize;
		api.Window.get_size = &detail::vk::Window::GetSize;
		api.Window.begin_frame = &detail::vk::Window::BeginFrame;
		api.Window.end_frame = &detail::vk::Window::EndFrame;
		return api;
	}
}
