#pragma once

#include "resource_manager.hpp"

#include <leaf/graphics/window.hpp>
#include <leaf/core/string.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/native_window.hpp>

#include <vulkan/vulkan.h>


namespace lf::vk {
	struct WindowVK {
		using front_t = Window;
		inline static ResourceManager<WindowVK> resource_manager;


		WindowVK(string_view title, dim2<u32> extent);
		~WindowVK();

		static handle<Window> create(string_view title, u32 width, u32 height);

		NativeWindow* native;
		VkSwapchainKHR vk_swapchain;
		VkSurfaceKHR vk_surface;

		struct swapchain_image {
			VkImage vk_image;
			VkImageView vk_image_view;
			// VkFramebuffer vk_framebuffer;
		};
		vector<swapchain_image> swapchain_images;

		static Window::Backend get_backend();


	};
}