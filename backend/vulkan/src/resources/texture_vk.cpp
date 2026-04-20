#include "texture_vk.hpp"

#include "vulkan_context.hpp"

#include <leaf/core/exception.hpp>

TextureBaseVK::TextureBaseVK(vulkan_context& ctx, VkImage vk_image, VkFormat vk_format, VkImageViewType vk_view_type) : ctx(ctx), vk_image(vk_image), vk_format(vk_format), vk_view_type(vk_view_type) {
	VkImageViewCreateInfo image_view_create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	image_view_create_info.image = vk_image;
	image_view_create_info.viewType = vk_view_type;
	image_view_create_info.format = vk_format;
	image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_create_info.subresourceRange.baseMipLevel = 0;
	image_view_create_info.subresourceRange.levelCount = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount = 1;


	if (VkResult result = vkCreateImageView(ctx.vk_device, &image_view_create_info, nullptr, &vk_image_view); result != VK_SUCCESS) {
		throw lf::runtime_exception("failed to create a Vulkan image view for a texture");
	}
}
TextureBaseVK::~TextureBaseVK() {
	vkDestroyImageView(ctx.vk_device, vk_image_view, nullptr);
}
