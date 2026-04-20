#include "queue_vk.hpp"

#include "../vulkan_context.hpp"

QueueVK::QueueVK(vulkan_context& ctx, VkQueue queue, u32 family_index, u32 queue_index, VkQueueFlags flags)
	: ctx(ctx)
	, vk_queue(queue)
	, flags(flags)
	, family_index(family_index)
	, queue_index(queue_index) {
	VkSemaphoreTypeCreateInfo vk_timeline_semaphore_type_create_info = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
	vk_timeline_semaphore_type_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	vk_timeline_semaphore_type_create_info.initialValue = 0;

	VkSemaphoreCreateInfo vk_semaphore_create_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	vk_semaphore_create_info.pNext = &vk_timeline_semaphore_type_create_info;

	if (VkResult result = vkCreateSemaphore(ctx.vk_device, &vk_semaphore_create_info, nullptr, &vk_timeline_semaphore); result != VK_SUCCESS) {
		lf::abort();
	}
}

QueueVK::~QueueVK() {
	if (vk_timeline_semaphore) {
		vkDestroySemaphore(ctx.vk_device, vk_timeline_semaphore, nullptr);
		vk_timeline_semaphore = VK_NULL_HANDLE;
	}
}
