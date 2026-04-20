#pragma once

#include "resource.hpp"

#include <leaf/graphics/detail/resource.hpp>
#include <vulkan/vulkan.h>

#include <atomic>

struct vulkan_context;

struct QueueVK : Resource {
	vulkan_context& ctx;
	QueueVK(vulkan_context& ctx, VkQueue queue, u32 family_index, u32 queue_index, VkQueueFlags flags);
	~QueueVK();

	VkQueue vk_queue = VK_NULL_HANDLE;
	VkSemaphore vk_timeline_semaphore = VK_NULL_HANDLE;
	std::atomic<u64> next_timeline_value = 1;
	VkQueueFlags flags = 0;
	u32 family_index = 0;
	u32 queue_index = 0;
};
struct QueueTimepoint {
	lf::view<const lf::queue> queue;
	u64 value = 0;
};
