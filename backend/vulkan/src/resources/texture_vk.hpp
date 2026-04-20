#pragma once

#include "resource.hpp"

#include <vulkan/vulkan.h>
#include <leaf/graphics/detail/resource.hpp>


namespace lf::resource {
	struct texture_base;
}
template<> struct lf::type_name_trait<lf::texture_base> { static constexpr lf::string_view value = "texture_base"; };

struct vulkan_context;


// It does not own the image. This is an abstraction to help determine if an
// Image is still alive by using the handle system. It does however own the
// Image view
struct TextureBaseVK : Resource {
	TextureBaseVK(vulkan_context& ctx, VkImage vk_image, VkFormat vk_format, VkImageViewType vk_view_type);
	~TextureBaseVK();
	vulkan_context& ctx;
	VkImage vk_image = VK_NULL_HANDLE;
	VkImageView vk_image_view = VK_NULL_HANDLE;
	VkFormat vk_format;
	VkImageViewType vk_view_type;
};


struct Texture2DVK : Resource {
	vulkan_context& ctx;

	lf::handle<lf::texture_base> base;
};
