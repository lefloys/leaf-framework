#pragma once

#include "object_allocator.hpp"

#include <vulkan/vulkan.h>
#include <memory>

struct WindowVK;

struct context {
	VkInstance vk_instance = VK_NULL_HANDLE;
	resource_pool<lf::resource::window, WindowVK> windows;

	void shutdown();
};

bool has_context();
void create_context();
context& get_context();
void destroy_context();
