#include <leaf/graphics/backends/vulkan.hpp>

#include "leaf/platform/api.hpp"
#include "resources/window_vk.hpp"
#include "vulkan_context.hpp"

lf::error GraphicsInit() {
	if (has_context()) {
		return lf::error::no_error;
	}

	create_context();
	context& ctx = get_context();

	if (!lf::Platform.platform_vulkan_supported || !lf::Platform.platform_vulkan_supported()) {
		destroy_context();
		return lf::error(lf::generic_errc::unknown, "the active platform backend reports that Vulkan is not supported");
	}

	u32 extension_count = 0;
	const char** extensions = lf::Platform.get_platform_vulkan_instance_extensions
		? lf::Platform.get_platform_vulkan_instance_extensions(extension_count)
		: nullptr;
	if (!extensions || extension_count == 0) {
		destroy_context();
		return lf::error(lf::generic_errc::unknown, "failed to query required Vulkan instance extensions from the platform backend");
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

	if (VkResult result = vkCreateInstance(&create_info, nullptr, &ctx.vk_instance); result != VK_SUCCESS) {
		destroy_context();
		return lf::error(lf::generic_errc::unknown, "failed to create Vulkan instance");
	}

	return lf::error::no_error;
}

void GraphicsExit() {
	if (!has_context()) {
		return;
	}

	destroy_context();
}

namespace lf {
	GraphicsAPI CreateVulkanGraphicsAPI() {
		GraphicsAPI api{};
		api.init = &GraphicsInit;
		api.exit = &GraphicsExit;
		api.Window.create = &Window::Create;
		api.Window.destroy = &Window::Destroy;
		api.Window.show = &Window::Show;
		api.Window.hide = &Window::Hide;
		api.Window.resize = &Window::Resize;
		api.Window.get_size = &Window::GetSize;
		api.Window.begin_frame = &Window::BeginFrame;
		api.Window.end_frame = &Window::EndFrame;
		return api;
	}
}
