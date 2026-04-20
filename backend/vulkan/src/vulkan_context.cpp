#include "vulkan_context.hpp"

#include "resources/window_vk.hpp"

#include <leaf/core/algorithm.hpp>
#include <leaf/core/cstddef.hpp>
#include <leaf/core/cstdlib.hpp>
#include <leaf/core/cstring.hpp>
#include <leaf/core/exception.hpp>
#include <leaf/core/iostream.hpp>
#include <leaf/core/memory.hpp>
#include <leaf/core/types.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/platform/api.hpp>

namespace {
	constexpr const char* k_validation_layer_name = "VK_LAYER_KHRONOS_validation";
	constexpr const char* k_swapchain_extension_name = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

	VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void*) {
		const char* severity = "info";
		if ((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0) {
			severity = "error";
		}
		else if ((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0) {
			severity = "warning";
		}
		else if ((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) != 0) {
			severity = "verbose";
		}

		lf::cerr << "leaf vulkan [" << severity << "]: " << callback_data->pMessage << '\n';
		return VK_FALSE;
	}

	bool has_instance_layer(const char* layer_name) {
		u32 layer_count = 0;
		if (vkEnumerateInstanceLayerProperties(&layer_count, nullptr) != VK_SUCCESS) {
			return false;
		}

		lf::vector<VkLayerProperties> layers(layer_count);
		if (layer_count != 0 && vkEnumerateInstanceLayerProperties(&layer_count, layers.data()) != VK_SUCCESS) {
			return false;
		}

		return lf::any_of(layers.begin(), layers.end(), [layer_name](const VkLayerProperties& layer) {
			return lf::strcmp(layer.layerName, layer_name) == 0;
			});
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

		return lf::any_of(extensions.begin(), extensions.end(), [extension_name](const VkExtensionProperties& extension) {
			return lf::strcmp(extension.extensionName, extension_name) == 0;
			});
	}

	bool has_device_extension(VkPhysicalDevice physical_device, const char* extension_name) {
		u32 extension_count = 0;
		if (vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr) != VK_SUCCESS) {
			return false;
		}

		lf::vector<VkExtensionProperties> extensions(extension_count);
		if (extension_count != 0 && vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, extensions.data()) != VK_SUCCESS) {
			return false;
		}

		return lf::any_of(extensions.begin(), extensions.end(), [extension_name](const VkExtensionProperties& extension) {
			return lf::strcmp(extension.extensionName, extension_name) == 0;
			});
	}

	VkDebugUtilsMessengerCreateInfoEXT make_debug_messenger_create_info() {
		VkDebugUtilsMessengerCreateInfoEXT create_info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		create_info.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		create_info.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.pfnUserCallback = &debug_messenger_callback;
		return create_info;
	}
}

lf::error create_vk_debug_messenger(vulkan_context& ctx) {
	PFN_vkCreateDebugUtilsMessengerEXT create_debug_utils_messenger =
		reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(ctx.vk_instance, "vkCreateDebugUtilsMessengerEXT"));
	if (!create_debug_utils_messenger) {
		return lf::error(lf::generic_errc::unknown, "VK_EXT_debug_utils was requested, but vkCreateDebugUtilsMessengerEXT is unavailable");
	}

	VkDebugUtilsMessengerCreateInfoEXT create_info = make_debug_messenger_create_info();
	if (VkResult result = create_debug_utils_messenger(ctx.vk_instance, &create_info, nullptr, &ctx.vk_debug_messenger); result != VK_SUCCESS) {
		return lf::error(lf::generic_errc::unknown, "failed to create Vulkan debug messenger");
	}

	return lf::error::no_error;
}

lf::unique_ptr<vulkan_context> context_ptr;

lf::error vulkan_context::init_physical_device() {
	u32 physical_device_count = 0;
	if (VkResult result = vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, nullptr); result != VK_SUCCESS) {
		return lf::error(lf::generic_errc::unknown, "failed to enumerate Vulkan physical devices");
	}
	if (physical_device_count == 0) {
		return lf::error(lf::generic_errc::unknown, "no Vulkan physical devices were found");
	}

	lf::vector<VkPhysicalDevice> devices(physical_device_count);
	if (VkResult result = vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, devices.data()); result != VK_SUCCESS) {
		return lf::error(lf::generic_errc::unknown, "failed to enumerate Vulkan physical devices");
	}

	for (VkPhysicalDevice candidate : devices) {
		if (lf::has_platform_backend() && !has_device_extension(candidate, k_swapchain_extension_name)) {
			continue;
		}

		vk_physical_device = candidate;
		return lf::error::no_error;
	}

	return lf::error(lf::generic_errc::unknown, lf::has_platform_backend()
		? "failed to find a Vulkan physical device with swapchain support and usable queues"
		: "failed to find a Vulkan physical device that could create a logical device");
}


lf::error vulkan_context::init_device() {
	if (lf::error err = init_physical_device()) { return err; }

	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, nullptr);

	lf::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, queue_family_properties.data());

	lf::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	lf::vector<lf::vector<f32>> queue_priorities;
	for (u32 family_index = 0; family_index < queue_family_count; ++family_index) {
		const VkQueueFamilyProperties& family = queue_family_properties[family_index];
		if (family.queueCount == 0) {
			continue;
		}

		queue_priorities.emplace_back(static_cast<size_t>(family.queueCount), 1.0f);

		VkDeviceQueueCreateInfo queue_create_info{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queue_create_info.queueFamilyIndex = family_index;
		queue_create_info.queueCount = family.queueCount;
		queue_create_info.pQueuePriorities = queue_priorities.back().data();
		queue_create_infos.push_back(queue_create_info);
	}

	if (queue_create_infos.empty()) {
		exit_physical_device();
		return lf::error(lf::generic_errc::unknown, "the Vulkan physical device does not expose any usable queues");
	}

	lf::vector<const char*> enabled_extensions;
	if (lf::has_platform_backend()) {
		enabled_extensions.push_back(k_swapchain_extension_name);
	}

	VkPhysicalDeviceFeatures features{};
	VkPhysicalDeviceTimelineSemaphoreFeatures vk_timeline_semaphore_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES };
	vk_timeline_semaphore_features.timelineSemaphore = VK_TRUE;
	VkPhysicalDeviceDynamicRenderingFeatures vk_dynamic_rendering_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
	vk_dynamic_rendering_features.dynamicRendering = VK_TRUE;
	vk_timeline_semaphore_features.pNext = &vk_dynamic_rendering_features;
	VkDeviceCreateInfo create_info{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	create_info.queueCreateInfoCount = static_cast<u32>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.enabledExtensionCount = static_cast<u32>(enabled_extensions.size());
	create_info.ppEnabledExtensionNames = enabled_extensions.data();
	create_info.pEnabledFeatures = &features;
	create_info.pNext = &vk_timeline_semaphore_features;

	if (Setting::DEBUG && has_instance_layer(k_validation_layer_name)) {
		create_info.enabledLayerCount = 1;
		create_info.ppEnabledLayerNames = &k_validation_layer_name;
	}

	if (VkResult result = vkCreateDevice(vk_physical_device, &create_info, nullptr, &vk_device); result != VK_SUCCESS) {
		exit_physical_device();
		return lf::error(lf::generic_errc::unknown, "failed to create the Vulkan logical device");
	}

	if (lf::error err = init_queues(queue_family_properties)) {
		exit_device();
		exit_physical_device();
		return err;
	}

	return lf::error::no_error;
}

lf::error vulkan_context::init_queues(lf::span<VkQueueFamilyProperties> queue_family_properties) {
	for (u32 family_index = 0; family_index < queue_family_properties.size(); ++family_index) {
		const VkQueueFamilyProperties& family = queue_family_properties[family_index];
		for (u32 queue_index = 0; queue_index < family.queueCount; ++queue_index) {
			VkQueue vk_queue_handle = VK_NULL_HANDLE;
			vkGetDeviceQueue(vk_device, family_index, queue_index, &vk_queue_handle);
			queues.push_back(queues_pool.create(*this, vk_queue_handle, family_index, queue_index, family.queueFlags));
		}
	}


	return lf::error::no_error;
}

void vulkan_context::exit_queues() {
	queues_pool.clear();
	queues.clear();
}

void vulkan_context::exit_device() {
	vkDestroyDevice(vk_device, nullptr);
	vk_device = VK_NULL_HANDLE;
}

void vulkan_context::exit_physical_device() {
	vk_physical_device = VK_NULL_HANDLE;
}

void vulkan_context::shutdown() {
	vkDeviceWaitIdle(vk_device);

	windows.clear_leaked_resources();
	queues_pool.clear_leaked_resources();
	framebuffers.clear_leaked_resources();
	command_buffers.clear_leaked_resources();
	texture_bases.clear_leaked_resources();


	exit_queues();
	exit_device();
	exit_physical_device();

	PFN_vkDestroyDebugUtilsMessengerEXT destroy_debug_utils_messenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vk_instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (destroy_debug_utils_messenger) { destroy_debug_utils_messenger(vk_instance, vk_debug_messenger, nullptr); }
	vk_debug_messenger = VK_NULL_HANDLE;

	vkDestroyInstance(vk_instance, nullptr);
	vk_instance = VK_NULL_HANDLE;
}


bool has_context() {
	return static_cast<bool>(context_ptr);
}
void assert_context() {
	if (!has_context()) {
		throw lf::runtime_exception("the Vulkan backend was used before lf::Init completed");
	}
}

vulkan_context& allocate_context() {
	if (!context_ptr) {
		context_ptr = lf::make_unique<vulkan_context>();
	}

	return *context_ptr;
}
void create_context(vulkan_context& ctx) {
	if (context_ptr.get() != &ctx) {
		lf::abort();
	}
}
void destroy_context(vulkan_context& ctx) {
	ctx.shutdown();
	if (context_ptr.get() == &ctx) {
		context_ptr.reset();
	}
}

vulkan_context& get_context() {
	assert_context();
	return *context_ptr;
}
