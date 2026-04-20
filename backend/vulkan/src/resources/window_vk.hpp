#pragma once

#include "resource.hpp"
#include "texture_vk.hpp"

#include <leaf/core/string.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/graphics/detail/resource.hpp>
#include <leaf/math/dim.hpp>
#include <leaf/platform/api.hpp>
#include <vulkan/vulkan.h>

struct vulkan_context;

struct WindowVK : Resource {
	struct frame_sync {
		VkSemaphore vk_image_available = VK_NULL_HANDLE;
		VkSemaphore vk_render_finished = VK_NULL_HANDLE;
		VkFence vk_in_flight = VK_NULL_HANDLE;
	};
	struct {
		VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;
		VkExtent2D vk_swapchain_extent{};
		VkFormat vk_swapchain_image_format = VK_FORMAT_UNDEFINED;
		lf::vector<lf::handle<lf::texture_base>> images;
	} Swapchain;


	WindowVK(vulkan_context& ctx, lf::string_view title, lf::dim2<i32> extent);
	~WindowVK();

	void create_swapchain();
	void destroy_swapchain();

	vulkan_context& ctx;

	lf::platform_window* platform_window = nullptr;
	VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
	VkQueue vk_present_queue = VK_NULL_HANDLE;

	lf::vector<lf::handle<lf::framebuffer>> framebuffers;
	lf::handle<lf::framebuffer> active_framebuffer;
	lf::vector<frame_sync> frames;

	u32 present_queue_family_index = 0;
	u32 current_frame = 0;
	u32 acquired_image_index = 0;
};

namespace Window {
	lf::handle<lf::window> Create(lf::string_view title, lf::dim2<i32> extent);
	void Destroy(lf::handle<lf::window> wnd);
	void Show(lf::view<lf::window> wnd);
	void Hide(lf::view<lf::window> wnd);
	void Resize(lf::view<lf::window> wnd, lf::dim2<i32> extent);
	lf::dim2<i32> GetSize(lf::view<const lf::window> wnd);
	void AcquireImage(lf::view<lf::window> wnd);
	lf::view<lf::framebuffer> GetFramebuffer(lf::view<lf::window> wnd);
	void Present(lf::view<lf::window> wnd);
	bool ShouldClose(lf::view<const lf::window> wnd);
}
