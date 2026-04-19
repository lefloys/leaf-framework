#pragma once

#include "object_allocator.hpp"
#include "resources/command_buffer_vk.hpp"
#include "resources/framebuffer_vk.hpp"
#include "resources/window_vk.hpp"

#include <leaf/core/vector.hpp>
#include <leaf/core/error.hpp>
#include <leaf/leaf.hpp>
#include <leaf/core/span.hpp>
#include <vulkan/vulkan.h>

template<typename>
struct vk_resource_traits;

struct QueueTimepoint {
	VkSemaphore vk_timeline_semaphore = VK_NULL_HANDLE;
	u64 value = 0;
};

struct vk_queue {
	VkQueue vk_queue_handle = VK_NULL_HANDLE;
	u32 family_index = 0;
	u32 queue_index = 0;
	VkQueueFlags flags = 0;
	VkSemaphore vk_timeline_semaphore = VK_NULL_HANDLE;
	u64 next_timeline_value = 1;
};

struct vulkan_context {
	VkInstance vk_instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT vk_debug_messenger = VK_NULL_HANDLE;
	VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE;
	VkDevice vk_device = VK_NULL_HANDLE;
	lf::vector<vk_queue> vk_queues;

	resource_pool<lf::resource::window, WindowVK> windows;
	resource_pool<lf::resource::framebuffer, FramebufferVK> framebuffers;
	resource_pool<lf::resource::command_buffer, CommandBufferVK> command_buffers;

	lf::error init_physical_device();
	lf::error init_device();
	lf::error init_queues(lf::span<VkQueueFamilyProperties> queue_family_properties);

	void exit_queues();
	void exit_device();
	void exit_physical_device();
	void shutdown();
};

template<>
struct vk_resource_traits<lf::window> {
	using resource_type = WindowVK;

	static auto& pool(vulkan_context& ctx) { return ctx.windows; }
	static const auto& pool(const vulkan_context& ctx) { return ctx.windows; }
};

template<>
struct vk_resource_traits<lf::framebuffer> {
	using resource_type = FramebufferVK;

	static auto& pool(vulkan_context& ctx) { return ctx.framebuffers; }
	static const auto& pool(const vulkan_context& ctx) { return ctx.framebuffers; }
};

template<>
struct vk_resource_traits<lf::command_buffer> {
	using resource_type = CommandBufferVK;

	static auto& pool(vulkan_context& ctx) { return ctx.command_buffers; }
	static const auto& pool(const vulkan_context& ctx) { return ctx.command_buffers; }
};


void assert_context();
bool has_context();
void create_context();
vulkan_context& get_context();
void destroy_context();
lf::error create_vk_debug_messenger(vulkan_context& ctx);

template<typename Tag>
typename vk_resource_traits<Tag>::resource_type& unhandle(lf::view<Tag> resource_view) {
	vulkan_context& ctx = get_context();
	return vk_resource_traits<Tag>::pool(ctx).get(resource_view);
}

template<typename Tag>
const typename vk_resource_traits<Tag>::resource_type& unhandle(lf::view<const Tag> resource_view) {
	const vulkan_context& ctx = get_context();
	return vk_resource_traits<Tag>::pool(ctx).get(resource_view);
}
