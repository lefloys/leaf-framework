#include "window_vk.hpp"

#include "../vulkan_context.hpp"
#include "framebuffer_vk.hpp"

#include "leaf/core/algorithm.hpp"
#include "leaf/core/array.hpp"
#include "leaf/core/cstddef.hpp"
#include "leaf/core/exception.hpp"
#include "leaf/core/limits.hpp"
#include "leaf/core/span.hpp"
#include "leaf/platform/api.hpp"

constexpr u32 PREFERRED_SWAPCHAIN_IMAGE_COUNT = 3;
constexpr u32 MAX_FRAMES_IN_FLIGHT = 3;

constexpr lf::array<VkFormat, 4> PREFERRED_SWAPCHAIN_FORMATS = { {
	VK_FORMAT_B8G8R8A8_UNORM,
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_B8G8R8A8_SRGB,
	VK_FORMAT_R8G8B8A8_SRGB,
} };
constexpr lf::array<VkColorSpaceKHR, 3> PREFERRED_SWAPCHAIN_COLOR_SPACES = { {
	VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
	VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT,
	VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT,
} };
constexpr lf::array<VkPresentModeKHR, 4> PREFERRED_PRESENT_MODE = { {
	VK_PRESENT_MODE_MAILBOX_KHR,
	VK_PRESENT_MODE_FIFO_RELAXED_KHR,
	VK_PRESENT_MODE_FIFO_KHR,
	VK_PRESENT_MODE_IMMEDIATE_KHR,
} };

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

WindowVK::WindowVK(vulkan_context& ctx, lf::string_view title, lf::dim2<i32> extent) : ctx(ctx) {
	if (!lf::has_platform_backend()) {
		throw lf::runtime_exception("no platform backend is active");
	}

	platform_window = lf::Platform.create_platform_window(title, extent);

	frames.resize(MAX_FRAMES_IN_FLIGHT);
	for (frame_sync& frame : frames) {
		VkSemaphoreCreateInfo semaphore_create_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		if (VkResult result = vkCreateSemaphore(ctx.vk_device, &semaphore_create_info, nullptr, &frame.vk_image_available); result != VK_SUCCESS) {
			throw lf::runtime_exception("failed to create a Vulkan semaphore for frame synchronization");
		}
		if (VkResult result = vkCreateSemaphore(ctx.vk_device, &semaphore_create_info, nullptr, &frame.vk_render_finished); result != VK_SUCCESS) {
			throw lf::runtime_exception("failed to create a Vulkan semaphore for frame synchronization");
		}

		VkFenceCreateInfo fence_create_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		if (VkResult result = vkCreateFence(ctx.vk_device, &fence_create_info, nullptr, &frame.vk_in_flight); result != VK_SUCCESS) {
			throw lf::runtime_exception("failed to create a Vulkan fence for frame synchronization");
		}
	}

	framebuffers.resize(MAX_FRAMES_IN_FLIGHT);
	for (lf::handle<lf::framebuffer>& fb : framebuffers) {
		fb = ctx.framebuffers.create(ctx);
	}

	lf::result<VkSurface> platform_surface = lf::Platform.create_platform_vulkan_surface(ctx.vk_instance, platform_window);
	if (!platform_surface) {
		lf::Platform.destroy_platform_window(platform_window);
		platform_window = nullptr;
		throw lf::runtime_exception("failed to create Vulkan window surface");
	}

	vk_surface = *platform_surface;
	create_swapchain();
}
WindowVK::~WindowVK() {
	// Ensure no in-flight work still references our per-frame semaphores/fences/swapchain.
	// The backend is still early; waiting here is acceptable to keep teardown valid.
	if (ctx.vk_device) {
		for (frame_sync& frame : frames) {
			if (frame.vk_in_flight) {
				vkWaitForFences(ctx.vk_device, 1, &frame.vk_in_flight, VK_TRUE, UINT64_MAX);
			}
		}
		vkDeviceWaitIdle(ctx.vk_device);
	}

	destroy_swapchain();
	vkDestroySurfaceKHR(ctx.vk_instance, vk_surface, nullptr);

	for (lf::handle<lf::framebuffer>& fb : framebuffers) {
		if (fb) {
			ctx.framebuffers.destroy(fb);
			fb = {};
		}
	}
	active_framebuffer = {};

	for (frame_sync& frame : frames) {
		vkDestroyFence(ctx.vk_device, frame.vk_in_flight, nullptr);
		vkDestroySemaphore(ctx.vk_device, frame.vk_render_finished, nullptr);
		vkDestroySemaphore(ctx.vk_device, frame.vk_image_available, nullptr);
		frame.vk_in_flight = VK_NULL_HANDLE;
		frame.vk_render_finished = VK_NULL_HANDLE;
		frame.vk_image_available = VK_NULL_HANDLE;
	}
	frames.clear();
	framebuffers.clear();
	lf::Platform.destroy_platform_window(platform_window);

	vk_surface = VK_NULL_HANDLE;
	platform_window = nullptr;
}

void WindowVK::create_swapchain() {
	bool present_queue_found = false;
	//for (lf::handle<lf::queue> queue_handle : ctx.queues) {
	//	const QueueVK& queue = ctx.unhandle(queue_handle);
	//	VkBool32 present_supported = VK_FALSE;
	//	if (VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(ctx.vk_physical_device, queue.family_index, vk_surface, &present_supported); result != VK_SUCCESS) {
	//		throw lf::runtime_exception("failed to query Vulkan present support for the window surface");
	//	}
	//	if (!present_supported) {
	//		continue;
	//	}
	//	vk_present_queue = queue.vk_queue;
	//	present_queue_family_index = queue.family_index;
	//	present_queue_found = true;
	//	break;
	//}

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
	lf::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
	if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.vk_physical_device, vk_surface, &surface_format_count, surface_formats.data()); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to query Vulkan surface formats");
	}


	u32 present_mode_count = 0;
	if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.vk_physical_device, vk_surface, &present_mode_count, nullptr); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to query Vulkan present modes");
	}
	lf::vector<VkPresentModeKHR> present_modes(present_mode_count);
	if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.vk_physical_device, vk_surface, &present_mode_count, present_modes.data()); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to query Vulkan present modes");
	}


	const VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(surface_formats);
	Swapchain.vk_swapchain_image_format = surface_format.format;
	Swapchain.vk_swapchain_extent = choose_swapchain_extent(surface_capabilities, lf::Platform.get_platform_window_extent(platform_window));

	VkSwapchainCreateInfoKHR swapchain_create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchain_create_info.surface = vk_surface;
	swapchain_create_info.minImageCount = choose_swapchain_image_count(surface_capabilities);
	swapchain_create_info.imageFormat = Swapchain.vk_swapchain_image_format;
	swapchain_create_info.imageColorSpace = surface_format.colorSpace;
	swapchain_create_info.imageExtent = Swapchain.vk_swapchain_extent;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.preTransform = surface_capabilities.currentTransform;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.presentMode = choose_swapchain_present_mode(present_modes);
	swapchain_create_info.clipped = VK_TRUE;

	if (VkResult result = vkCreateSwapchainKHR(ctx.vk_device, &swapchain_create_info, nullptr, &Swapchain.vk_swapchain); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to create Vulkan swapchain");
	}

	u32 swapchain_image_count = 0;
	if (VkResult result = vkGetSwapchainImagesKHR(ctx.vk_device, Swapchain.vk_swapchain, &swapchain_image_count, nullptr); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to query Vulkan swapchain images");
	}

	lf::vector<VkImage> vk_images(swapchain_image_count);
	if (VkResult result = vkGetSwapchainImagesKHR(ctx.vk_device, Swapchain.vk_swapchain, &swapchain_image_count, vk_images.data()); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to query Vulkan swapchain images");
	}

	Swapchain.images.reserve(vk_images.size());
	for (auto i : lf::range(0, vk_images.size())) {
		Swapchain.images.emplace_back(ctx.texture_bases.create(ctx, vk_images[i], Swapchain.vk_swapchain_image_format, VK_IMAGE_VIEW_TYPE_2D));
	}
}

void WindowVK::destroy_swapchain() {
	for (auto& image : Swapchain.images) {
		ctx.texture_bases.destroy(image);
		image = {};
	}
	Swapchain.images.clear();

	vkDestroySwapchainKHR(ctx.vk_device, Swapchain.vk_swapchain, nullptr);
	Swapchain.vk_swapchain = VK_NULL_HANDLE;

	vk_present_queue = VK_NULL_HANDLE;
	Swapchain.vk_swapchain_extent = {};
	Swapchain.vk_swapchain_image_format = VK_FORMAT_UNDEFINED;
	acquired_image_index = 0;
	active_framebuffer = {};
}

namespace Window {
	lf::handle<lf::window> create(vulkan_context& ctx, lf::string_view title, lf::dim2<i32> extent) {
		return ctx.windows.create(ctx, title, extent);
	}
	lf::handle<lf::window> Create(lf::string_view title, lf::dim2<i32> extent) {
		assert_context();
		return create(get_context(), title, extent);
	}

	void destroy(vulkan_context& ctx, lf::handle<lf::window> wnd) {
		ctx.windows.destroy(wnd);
	}
	void Destroy(lf::handle<lf::window> wnd) {
		assert_context();
		destroy(get_context(), wnd);
	}

	void show(vulkan_context& ctx, WindowVK& wnd) {
		lf::Platform.show_platform_window(wnd.platform_window);
	}
	void Show(lf::view<lf::window> wnd) {
		auto& ctx = get_context();
		show(ctx, ctx.unhandle(wnd));
	}

	void hide(vulkan_context& ctx, WindowVK& wnd) {
		lf::Platform.hide_platform_window(wnd.platform_window);
	}
	void Hide(lf::view<lf::window> wnd) {
		auto& ctx = get_context();
		hide(ctx, ctx.unhandle(wnd));
	}

	void resize(vulkan_context& ctx, WindowVK& wnd, lf::dim2<i32> extent) {
		lf::Platform.set_platform_window_extent(wnd.platform_window, extent);
	}
	void Resize(lf::view<lf::window> wnd, lf::dim2<i32> extent) {
		auto& ctx = get_context();
		resize(ctx, ctx.unhandle(wnd), extent);
	}

	lf::dim2<i32> get_size(vulkan_context& ctx, const WindowVK& wnd) {
		return lf::Platform.get_platform_window_extent(wnd.platform_window);
	}
	lf::dim2<i32> GetSize(lf::view<const lf::window> wnd) {
		auto& ctx = get_context();
		return get_size(ctx, ctx.unhandle(wnd));
	}

	void acquire_image(vulkan_context& ctx, WindowVK& wnd) {
		if (wnd.active_framebuffer) {
			throw lf::runtime_exception("BeginFrame was called twice without a matching EndFrame");
		}

		const u32 frame_slot = wnd.current_frame % static_cast<u32>(wnd.frames.size());
		WindowVK::frame_sync& frame = wnd.frames[frame_slot];
		if (VkResult result = vkWaitForFences(ctx.vk_device, 1, &frame.vk_in_flight, VK_TRUE, UINT64_MAX); result != VK_SUCCESS) {
			throw lf::runtime_exception("failed to wait for the Vulkan in-flight fence");
		}
		if (VkResult result = vkResetFences(ctx.vk_device, 1, &frame.vk_in_flight); result != VK_SUCCESS) {
			throw lf::runtime_exception("failed to reset the Vulkan in-flight fence");
		}

		VkResult acquire_result = vkAcquireNextImageKHR(ctx.vk_device, wnd.Swapchain.vk_swapchain, UINT64_MAX, frame.vk_image_available, VK_NULL_HANDLE, &wnd.acquired_image_index);
		if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR) {
			wnd.destroy_swapchain();
			wnd.create_swapchain();
			acquire_result = vkAcquireNextImageKHR(ctx.vk_device, wnd.Swapchain.vk_swapchain, UINT64_MAX, frame.vk_image_available, VK_NULL_HANDLE, &wnd.acquired_image_index);
		}
		if (acquire_result != VK_SUCCESS) {
			throw lf::runtime_exception("failed to acquire the next Vulkan swapchain image");
		}

		wnd.active_framebuffer = wnd.framebuffers[frame_slot];
	}
	void AcquireImage(lf::view<lf::window> wnd) {
		auto& ctx = get_context();
		acquire_image(ctx, unhandle(ctx, wnd));
	}

	lf::view<lf::framebuffer> GetFramebuffer(lf::view<lf::window> wnd) {
		return unhandle(get_context(), wnd).active_framebuffer;
	}

	void present(vulkan_context& ctx, WindowVK& wnd) {
		if (!wnd.active_framebuffer) {
			throw lf::runtime_exception("EndFrame was called without a matching BeginFrame");
		}

		const u32 frame_slot = wnd.current_frame % static_cast<u32>(wnd.frames.size());
		WindowVK::frame_sync& frame = wnd.frames[frame_slot];
		FramebufferVK& fb = ctx.framebuffers.get(wnd.active_framebuffer);

		//if (VkResult result = vkResetCommandPool(ctx.vk_device, fb.vk_command_pool, 0); result != VK_SUCCESS) {
		//	throw lf::runtime_exception("failed to reset the Vulkan command pool for the frame");
		//}

		//VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		//begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		//if (VkResult result = vkBeginCommandBuffer(fb.vk_primary_command_buffer, &begin_info); result != VK_SUCCESS) {
		//	throw lf::runtime_exception("failed to begin recording the Vulkan primary command buffer for the frame");
		//}

		//const VkImage swapchain_image = wnd.swapchain_images[wnd.acquired_image_index].vk_image;

		//VkImageMemoryBarrier to_color_attachment = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		//to_color_attachment.srcAccessMask = 0;
		//to_color_attachment.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		//to_color_attachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		//to_color_attachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//to_color_attachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//to_color_attachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//to_color_attachment.image = swapchain_image;
		//to_color_attachment.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//to_color_attachment.subresourceRange.baseMipLevel = 0;
		//to_color_attachment.subresourceRange.levelCount = 1;
		//to_color_attachment.subresourceRange.baseArrayLayer = 0;
		//to_color_attachment.subresourceRange.layerCount = 1;

		//vkCmdPipelineBarrier(fb.vk_primary_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_color_attachment);

		//if (!fb.submitted_secondary_buffers.empty()) {
		//	vkCmdExecuteCommands(fb.vk_primary_command_buffer, static_cast<u32>(fb.submitted_secondary_buffers.size()), fb.submitted_secondary_buffers.data());
		//}

		//VkImageMemoryBarrier to_present = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		//to_present.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		//to_present.dstAccessMask = 0;
		//to_present.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		//to_present.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//to_present.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//to_present.image = swapchain_image;
		//to_present.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//to_present.subresourceRange.baseMipLevel = 0;
		//to_present.subresourceRange.levelCount = 1;
		//to_present.subresourceRange.baseArrayLayer = 0;
		//to_present.subresourceRange.layerCount = 1;

		//vkCmdPipelineBarrier(fb.vk_primary_command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_present);

		//if (VkResult result = vkEndCommandBuffer(fb.vk_primary_command_buffer); result != VK_SUCCESS) {
		//	throw lf::runtime_exception("failed to finish recording the Vulkan primary command buffer for the frame");
		//}

		//VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		//VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		//submit_info.waitSemaphoreCount = 1;
		//submit_info.pWaitSemaphores = &frame.vk_image_available;
		//submit_info.pWaitDstStageMask = wait_stages;
		//submit_info.commandBufferCount = 1;
		//submit_info.pCommandBuffers = &fb.vk_primary_command_buffer;
		//submit_info.signalSemaphoreCount = 1;
		//submit_info.pSignalSemaphores = &frame.vk_render_finished;

		//const QueueVK& graphics_queue = unhandle(wnd.ctx, lf::view<const lf::queue>(ctx.graphics_queue));
		//if (VkResult result = vkQueueSubmit(graphics_queue.vk_queue, 1, &submit_info, frame.vk_in_flight); result != VK_SUCCESS) {
		//	throw lf::runtime_exception("failed to submit the Vulkan primary command buffer for the frame");
		//}

		//fb.submitted_secondary_buffers.clear();

		//VkSwapchainKHR swapchain = wnd.vk_swapchain;
		//VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		//present_info.waitSemaphoreCount = 1;
		//present_info.pWaitSemaphores = &frame.vk_render_finished;
		//present_info.swapchainCount = 1;
		//present_info.pSwapchains = &swapchain;
		//present_info.pImageIndices = &wnd.acquired_image_index;

		//VkResult result = vkQueuePresentKHR(wnd.vk_present_queue, &present_info);
		//if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		//	wnd.destroy_swapchain();
		//	wnd.create_swapchain();
		//}
		//else if (result != VK_SUCCESS) {
		//	throw lf::runtime_exception("failed to present the Vulkan swapchain image");
		//}

		wnd.framebuffers[frame_slot] = ctx.framebuffers.bump_id(wnd.framebuffers[frame_slot]);
		wnd.active_framebuffer = {};
		++wnd.current_frame;
	}
	void Present(lf::view<lf::window> wnd) {
		auto& ctx = get_context();
		present(ctx, unhandle(ctx, wnd));
	}


	bool should_close(const WindowVK& wnd) {
		return lf::Platform.platform_window_should_close(wnd.platform_window);
	}
	bool ShouldClose(lf::view<const lf::window> wnd) {
		assert_context();
		return should_close(get_context().unhandle(wnd));
	}
}
