#ifndef LEAF_GRAPHICS_TEXTURE_HPP
#define LEAF_GRAPHICS_TEXTURE_HPP

#include <leaf/graphics/resource.hpp>
namespace rt::Texture {
	handle<texture> Create();
	void Destroy(handle<texture> texture);
	void Resize(view<texture> texture, rt_texture_type type, rt_format format, rt_extent_3d extent, u64 mip_count = 1);
} // namespace rt::Texture

#endif /* LEAF_GRAPHICS_TEXTURE_HPP */
