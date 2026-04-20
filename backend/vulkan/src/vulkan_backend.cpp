#include <leaf/graphics/backends/vulkan.hpp>

#include "leaf/core/cstring.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/platform/api.hpp"
#include "resources/command_buffer_vk.hpp"
#include "resources/framebuffer_vk.hpp"
#include "resources/window_vk.hpp"
#include "vulkan_context.hpp"

namespace {
	constexpr const char* k_validation_layer_name = "VK_LAYER_KHRONOS_validation";
	constexpr const char* k_swapchain_extension_name = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	constexpr const char* k_debug_utils_extension_name = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	bool has_instance_layer(const char* layer_name) {
		u32 layer_count = 0;
		if (vkEnumerateInstanceLayerProperties(&layer_count, nullptr) != VK_SUCCESS) {
			return false;
		}

		lf::vector<VkLayerProperties> layers(layer_count);
		if (layer_count != 0 && vkEnumerateInstanceLayerProperties(&layer_count, layers.data()) != VK_SUCCESS) {
			return false;
		}

		for (const VkLayerProperties& layer : layers) {
			if (lf::strcmp(layer.layerName, layer_name) == 0) {
				return true;
			}
		}

		return false;
	}

	bool has_instance_extension(const char* extension_name) {
		u32 extension_count = 0;
		if (vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr) != VK_SUCCESS) {
			return false;
		}

		lf::vector<VkExtensionProperties> extensions(extension_count);
		if (extension_count != 0 && vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data()) != VK_SUCCESS) {
			return false;
		}

		for (const VkExtensionProperties& extension : extensions) {
			if (lf::strcmp(extension.extensionName, extension_name) == 0) {
				return true;
			}
		}

		return false;
	}
}

lf::error graphics_init() {
	if (has_context()) {
		return lf::error::no_error;
	}

	vulkan_context& ctx = allocate_context();
	create_context(ctx);
	const bool has_platform = lf::has_platform_backend();
	const bool debug =
#ifdef LEAF_DEBUG
		true;
#else
		false;
#endif

	if (has_platform && !lf::Platform.platform_vulkan_supported()) {
		destroy_context(ctx);
		return lf::error(lf::generic_errc::unknown, "the active platform backend reports that Vulkan is not supported");
	}

	u32 extension_count = 0;
	const char** extensions = has_platform
		? lf::Platform.get_platform_vulkan_instance_extensions(extension_count)
		: nullptr;
	if (has_platform && (!extensions || extension_count == 0)) {
		destroy_context(ctx);
		return lf::error(lf::generic_errc::unknown, "failed to query required Vulkan instance extensions from the platform backend");
	}
	if (debug && !has_instance_layer(k_validation_layer_name)) {
		destroy_context(ctx);
		return lf::error(lf::generic_errc::unknown, "debug mode requested Vulkan validation layers, but VK_LAYER_KHRONOS_validation is unavailable");
	}
	if (debug && !has_instance_extension(k_debug_utils_extension_name)) {
		destroy_context(ctx);
		return lf::error(lf::generic_errc::unknown, "debug mode requested VK_EXT_debug_utils, but the extension is unavailable");
	}

	lf::vector<const char*> enabled_extensions;
	if (has_platform) {
		enabled_extensions.assign(extensions, extensions + extension_count);
	}
	if (debug) {
		enabled_extensions.push_back(k_debug_utils_extension_name);
	}

	lf::vector<const char*> enabled_layers;
	if (debug) {
		enabled_layers.push_back(k_validation_layer_name);
	}

	VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app_info.pApplicationName = "leaf-framework";
	app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	app_info.pEngineName = "leaf";
	app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = static_cast<u32>(enabled_extensions.size());
	create_info.ppEnabledExtensionNames = enabled_extensions.data();
	create_info.enabledLayerCount = static_cast<u32>(enabled_layers.size());
	create_info.ppEnabledLayerNames = enabled_layers.data();

	if (VkResult result = vkCreateInstance(&create_info, nullptr, &ctx.vk_instance); result != VK_SUCCESS) {
		destroy_context(ctx);
		return lf::error(lf::generic_errc::unknown, "failed to create Vulkan instance");
	}

	if (debug) {
		if (lf::error err = create_vk_debug_messenger(ctx)) {
			destroy_context(ctx);
			return err;
		}
	}

	if (lf::error err = ctx.init_device()) {
		destroy_context(ctx);
		return err;
	}

	return lf::error::no_error;
}

void graphics_exit() {
	if (!has_context()) {
		return;
	}

	destroy_context(get_context());
}

namespace lf {
	GraphicsAPI CreateVulkanGraphicsAPI() {
		GraphicsAPI api{};
		api.init = &graphics_init;
		api.exit = &graphics_exit;
		api.Window.create = &::Window::Create;
		api.Window.destroy = &::Window::Destroy;
		api.Window.show = &::Window::Show;
		api.Window.hide = &::Window::Hide;
		api.Window.resize = &::Window::Resize;
		api.Window.get_size = &::Window::GetSize;
		api.Window.acquire_image = &::Window::AcquireImage;
		api.Window.get_framebuffer = &::Window::GetFramebuffer;
		api.Window.present = &::Window::Present;
		api.Framebuffer.destroy = &Framebuffer::Destroy;
		api.Framebuffer.submit = &Framebuffer::Submit;
		api.Framebuffer.flush = &Framebuffer::Flush;
		return api;
	}
}
