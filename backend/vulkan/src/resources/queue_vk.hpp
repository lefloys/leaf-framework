#pragma once

#include "resource.hpp"

#include <vulkan/vulkan.h>

struct QueueVK : Resource {
	QueueVK(VkQueue queue);
	~QueueVK();

	VkQueue vk_queue = VK_NULL_HANDLE;
	VkSemaphore vk_semaphore = VK_NULL_HANDLE;
};