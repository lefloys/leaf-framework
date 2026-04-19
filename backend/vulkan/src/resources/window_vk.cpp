#include "window_vk.hpp"

#include "../vulkan_context.hpp"

#include "leaf/core/algorithm.hpp"
#include "leaf/core/array.hpp"
#include "leaf/core/cstddef.hpp"
#include "leaf/core/exception.hpp"
#include "leaf/core/limits.hpp"
#include "leaf/core/span.hpp"
#include "leaf/platform/api.hpp"

constexpr u32 PREFERRED_SWAPCHAIN_IMAGE_COUNT = 3;
constexpr u32 MAX_FRAMES_IN_FLIGHT = 3;

constexpr lf::array<VkFormat, 4> PREFERRED_SWAPCHAIN_FORMATS = {{
	VK_FORMAT_B8G8R8A8_UNORM,
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_B8G8R8A8_SRGB,
	VK_FORMAT_R8G8B8A8_SRGB,
}};
constexpr lf::array<VkColorSpaceKHR, 3> PREFERRED_SWAPCHAIN_COLOR_SPACES = {{
	VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
	VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT,
	VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT,
}};
constexpr lf::array<VkPresentModeKHR, 4> PREFERRED_PRESENT_MODE = {{
	VK_PRESENT_MODE_MAILBOX_KHR,
	VK_PRESENT_MODE_FIFO_RELAXED_KHR,
	VK_PRESENT_MODE_FIFO_KHR,
	VK_PRESENT_MODE_IMMEDIATE_KHR,
}};

VkFormat choose_swapchain_format(lf::span<const VkSurfaceFormatKHR> available_formats) {
	for (const VkFormat preferred_format : PREFERRED_SWAPCHAIN_FORMATS) {
		for (const VkSurfaceFormatKHR& available_format : available_formats) {
			if (available_format.format == preferred_format) {
				return available_format.format;
			}
		}
	}

	throw lf::runtime_exception("none of the preferred Vulkan surface formats are supported by the swapchain surface");
}
VkColorSpaceKHR choose_swapchain_color_space(lf::span<const VkSurfaceFormatKHR> available_formats) {
	for (const VkColorSpaceKHR preferred_color_space : PREFERRED_SWAPCHAIN_COLOR_SPACES) {
		for (const VkSurfaceFormatKHR& available_format : available_formats) {
			if (available_format.colorSpace == preferred_color_space) {
				return available_format.colorSpace;
			}
		}
	}

	throw lf::runtime_exception("none of the preferred Vulkan color spaces are supported by the swapchain surface");
}
VkSurfaceFormatKHR choose_swapchain_surface_format(lf::span<const VkSurfaceFormatKHR> available_formats) {
	const VkFormat preferred_format = choose_swapchain_format(available_formats);
	const VkColorSpaceKHR preferred_color_space = choose_swapchain_color_space(available_formats);

	for (const VkSurfaceFormatKHR& available_format : available_formats) {
		if (available_format.format == preferred_format && available_format.colorSpace == preferred_color_space) {
			return available_format;
		}
	}

	throw lf::runtime_exception("the preferred Vulkan format and color space are not supported together by the swapchain surface");
}
VkPresentModeKHR choose_swapchain_present_mode(lf::span<const VkPresentModeKHR> available_present_modes) {
	for (const VkPresentModeKHR preferred_present_mode : PREFERRED_PRESENT_MODE) {
		for (const VkPresentModeKHR available_present_mode : available_present_modes) {
			if (available_present_mode == preferred_present_mode) {
				return available_present_mode;
			}
		}
	}

	throw lf::runtime_exception("none of the preferred Vulkan present modes are supported by the swapchain surface");
}
VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& capabilities, lf::dim2<i32> window_extent) {
	if (capabilities.currentExtent.width != lf::numeric_limits<u32>::max()) {
		return capabilities.currentExtent;
	}

	VkExtent2D extent{};
	extent.width = lf::clamp(static_cast<u32>(window_extent.width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	extent.height = lf::clamp(static_cast<u32>(window_extent.height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	return extent;
}

u32 choose_swapchain_image_count(const VkSurfaceCapabilitiesKHR& capabilities) {
	u32 image_count = lf::max(capabilities.minImageCount, PREFERRED_SWAPCHAIN_IMAGE_COUNT);
	if (capabilities.maxImageCount != 0) {
		image_count = lf::min(image_count, capabilities.maxImageCount);
	}

	return image_count;
}

WindowVK::WindowVK(lf::string_view title, lf::dim2<i32> extent) {
	vulkan_context& ctx = get_context();

	if (!lf::has_platform_backend()) {
		throw lf::runtime_exception("no platform backend is active");
	}

	platform_window = lf::Platform.create_platform_window(title, extent);

	lf::result<lf::VkSurface> platform_surface = lf::Platform.create_platform_vulkan_surface(ctx.vk_instance, platform_window);
	if (!platform_surface) {
		lf::Platform.destroy_platform_window(platform_window);
		platform_window = nullptr;
		throw lf::runtime_exception("failed to create Vulkan window surface");
	}

	vk_surface = *platform_surface;
	create_swapchain();
}
WindowVK::~WindowVK() {
	vulkan_context& ctx = get_context();

	destroy_swapchain();
	vkDestroySurfaceKHR(ctx.vk_instance, vk_surface, nullptr);
	lf::Platform.destroy_platform_window(platform_window);

	vk_surface = VK_NULL_HANDLE;
	platform_window = nullptr;
}

void WindowVK::create_swapchain() {
	vulkan_context& ctx = get_context();

	bool present_queue_found = false;
	for (const vk_queue& queue : ctx.vk_queues) {
		VkBool32 present_supported = VK_FALSE;
		if (VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(ctx.vk_physical_device, queue.family_index, vk_surface, &present_supported); result != VK_SUCCESS) {
			throw lf::runtime_exception("failed to query Vulkan present support for the window surface");
		}

		if (!present_supported) {
			continue;
		}

		vk_present_queue = queue.vk_queue_handle;
		vk_present_queue_family_index = queue.family_index;
		present_queue_found = true;
		break;
	}

	if (!present_queue_found) {
		throw lf::runtime_exception("failed to find a Vulkan queue family that can present to the window surface");
	}

	VkSurfaceCapabilitiesKHR surface_capabilities{};
	if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.vk_physical_device, vk_surface, &surface_capabilities); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to query Vulkan surface capabilities");
	}

	u32 surface_format_count = 0;
	if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.vk_physical_device, vk_surface, &surface_format_count, nullptr); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to query Vulkan surface formats");
	}
	if (surface_format_count == 0) {
		throw lf::runtime_exception("the Vulkan surface does not report any supported surface formats");
	}

	lf::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
	if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.vk_physical_device, vk_surface, &surface_format_count, surface_formats.data()); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to query Vulkan surface formats");
	}

	u32 present_mode_count = 0;
	if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.vk_physical_device, vk_surface, &present_mode_count, nullptr); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to query Vulkan present modes");
	}
	if (present_mode_count == 0) {
		throw lf::runtime_exception("the Vulkan surface does not report any present modes");
	}

	lf::vector<VkPresentModeKHR> present_modes(present_mode_count);
	if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.vk_physical_device, vk_surface, &present_mode_count, present_modes.data()); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to query Vulkan present modes");
	}

	const VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(surface_formats);
	vk_swapchain_image_format = surface_format.format;
	vk_swapchain_extent = choose_swapchain_extent(surface_capabilities, lf::Platform.get_platform_window_extent(platform_window));

	VkSwapchainCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	create_info.surface = vk_surface;
	create_info.minImageCount = choose_swapchain_image_count(surface_capabilities);
	create_info.imageFormat = vk_swapchain_image_format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = vk_swapchain_extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.preTransform = surface_capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = choose_swapchain_present_mode(present_modes);
	create_info.clipped = VK_TRUE;

	if (VkResult result = vkCreateSwapchainKHR(ctx.vk_device, &create_info, nullptr, &vk_swapchain); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to create Vulkan swapchain");
	}

	u32 swapchain_image_count = 0;
	if (VkResult result = vkGetSwapchainImagesKHR(ctx.vk_device, vk_swapchain, &swapchain_image_count, nullptr); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to query Vulkan swapchain images");
	}
	if (swapchain_image_count == 0) {
		throw lf::runtime_exception("the Vulkan swapchain did not return any images");
	}

	lf::vector<VkImage> vk_images(swapchain_image_count);
	if (VkResult result = vkGetSwapchainImagesKHR(ctx.vk_device, vk_swapchain, &swapchain_image_count, vk_images.data()); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to query Vulkan swapchain images");
	}

	swapchain_images.resize(swapchain_image_count);
	for (lf::size_t image_index = 0; image_index < swapchain_images.size(); ++image_index) {
		swapchain_image& image = swapchain_images[image_index];
		image.vk_image = vk_images[image_index];

		VkImageViewCreateInfo image_view_create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		image_view_create_info.image = image.vk_image;
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.format = vk_swapchain_image_format;
		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.layerCount = 1;

		if (VkResult result = vkCreateImageView(ctx.vk_device, &image_view_create_info, nullptr, &image.vk_image_view); result != VK_SUCCESS) {
			throw lf::runtime_exception("failed to create a Vulkan swapchain image view");
		}
	}
}

void WindowVK::destroy_swapchain() {
	vulkan_context& ctx = get_context();

	for (swapchain_image& image : swapchain_images) {
		vkDestroyImageView(ctx.vk_device, image.vk_image_view, nullptr);
		image.vk_image_view = VK_NULL_HANDLE;
		

		image.vk_image = VK_NULL_HANDLE;
		image.framebuffer = {};
	}

	swapchain_images.clear();

	vkDestroySwapchainKHR(ctx.vk_device, vk_swapchain, nullptr);
	vk_swapchain = VK_NULL_HANDLE;

	vk_present_queue = VK_NULL_HANDLE;
	vk_swapchain_extent = {};
	vk_swapchain_image_format = VK_FORMAT_UNDEFINED;
}

namespace Window {
	lf::handle<lf::window> Create(lf::string_view title, lf::dim2<i32> extent) {
		assert_context();
		return get_context().windows.create(title, extent);
	}

	void Destroy(lf::handle<lf::window> wnd) {
		assert_context();
		get_context().windows.destroy(wnd);
	}

	void Show(lf::view<lf::window> wnd) {
		assert_context();
		lf::Platform.show_platform_window(unhandle(wnd).platform_window);
	}

	void Hide(lf::view<lf::window> wnd) {
		assert_context();
		lf::Platform.hide_platform_window(unhandle(wnd).platform_window);
	}

	void Resize(lf::view<lf::window> wnd, lf::dim2<i32> extent) {
		assert_context();
		lf::Platform.set_platform_window_extent(unhandle(wnd).platform_window, extent);
	}

	lf::dim2<i32> GetSize(lf::view<const lf::window> wnd) {
		assert_context();
		return lf::Platform.get_platform_window_extent(unhandle(wnd).platform_window);
	}

	void AcquireImage(lf::view<lf::window> wnd) {
		assert_context();
		throw lf::runtime_exception("Window::AcquireImage is not implemented until the synchronization model exists");
	}

	void Present(lf::view<lf::window> wnd) {
		assert_context();
		throw lf::runtime_exception("Window::Present is not implemented until the synchronization model exists");
	}
}
