#pragma once

#include "object_allocator.hpp"
#include "resources/command_buffer_vk.hpp"
#include "resources/framebuffer_vk.hpp"
#include "resources/queue_vk.hpp"
#include "resources/window_vk.hpp"
#include "resources/texture_vk.hpp"

#include <leaf/core/vector.hpp>
#include <leaf/core/error.hpp>
#include <leaf/leaf.hpp>
#include <leaf/core/span.hpp>
#include <vulkan/vulkan.h>

template<typename>
struct vk_resource_traits;


struct vulkan_context {
	VkInstance vk_instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT vk_debug_messenger = VK_NULL_HANDLE;
	VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE;
	VkDevice vk_device = VK_NULL_HANDLE;
	lf::vector<lf::handle<lf::queue>> queues;

	resource_pool<lf::queue, QueueVK> queues_pool;
	resource_pool<lf::window, WindowVK> windows;
	resource_pool<lf::framebuffer, FramebufferVK> framebuffers;
	resource_pool<lf::command_buffer, CommandBufferVK> command_buffers;
	resource_pool<lf::texture_base, TextureBaseVK> texture_bases;

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
struct vk_resource_traits<lf::queue> {
	using resource_type = QueueVK;

	static auto& pool(vulkan_context& ctx) { return ctx.queues_pool; }
	static const auto& pool(const vulkan_context& ctx) { return ctx.queues_pool; }
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
template<typename Tag>
typename vk_resource_traits<Tag>::resource_type& unhandle(vulkan_context& ctx, lf::view<Tag> resource_view) {
	return vk_resource_traits<Tag>::pool(ctx).get(resource_view);
}

template<typename Tag>
const typename vk_resource_traits<Tag>::resource_type& unhandle(const vulkan_context& ctx, lf::view<const Tag> resource_view) {
	return vk_resource_traits<Tag>::pool(ctx).get(resource_view);
}


void assert_context();
bool has_context();

vulkan_context& allocate_context();
void create_context(vulkan_context& ctx);
vulkan_context& get_context();
void destroy_context(vulkan_context& ctx);

lf::error create_vk_debug_messenger(vulkan_context& ctx);
