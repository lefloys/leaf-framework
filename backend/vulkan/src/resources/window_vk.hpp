#pragma once

#include "resource.hpp"

#include "leaf/core/string.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/graphics/detail/resource.hpp"
#include "leaf/math/dim.hpp"
#include "leaf/platform/api.hpp"

#include <vulkan/vulkan.h>

struct vulkan_context;

struct WindowVK : Resource {
	struct swapchain_image {
		VkImage vk_image = VK_NULL_HANDLE;
		VkImageView vk_image_view = VK_NULL_HANDLE;
		lf::handle<lf::framebuffer> framebuffer{};
	};
	vulkan_context& ctx;
	lf::platform_window* platform_window = nullptr;
	VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
	VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;
	VkFormat vk_swapchain_image_format = VK_FORMAT_UNDEFINED;
	VkExtent2D vk_swapchain_extent{};
	VkQueue vk_present_queue = VK_NULL_HANDLE;
	u32 vk_present_queue_family_index = 0;
	lf::vector<swapchain_image> swapchain_images;

	WindowVK(vulkan_context& ctx, lf::string_view title, lf::dim2<i32> extent);
	~WindowVK();

	void create_swapchain();
	void destroy_swapchain();
};

namespace Window {
	lf::handle<lf::window> Create(lf::string_view title, lf::dim2<i32> extent);
	void Destroy(lf::handle<lf::window> wnd);
	void Show(lf::view<lf::window> wnd);
	void Hide(lf::view<lf::window> wnd);
	void Resize(lf::view<lf::window> wnd, lf::dim2<i32> extent);
	lf::dim2<i32> GetSize(lf::view<const lf::window> wnd);
	void AcquireImage(lf::view<lf::window> wnd);
	void Present(lf::view<lf::window> wnd);
}
