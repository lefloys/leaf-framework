#ifndef LEAF_GRAPHICS_TEXTURE_HPP
#define LEAF_GRAPHICS_TEXTURE_HPP

#include <leaf/graphics/resource.hpp>
namespace rt::Texture {
		handle<texture> Create();
		void Destroy(handle<texture> texture);
		timepoint Copy(view<queue> queue, view<texture> src_texture, u32 src_mip, view<texture> dst_texture, u32 dst_mip);
		timepoint Data(view<queue> queue, view<texture> texture, rt_texture_type type, u32 mip,
					   u32 offset_x, u32 offset_y, u32 offset_z, rt_format format,
					   const void* data);
		timepoint Subcopy(view<queue> queue, view<texture> src_texture, u32 src_mip, u32 src_x,
						  u32 src_y, u32 src_z, view<texture> dst_texture, u32 dst_mip, u32 dst_x,
						  u32 dst_y, u32 dst_z, u32 width, u32 height, u32 depth);
		timepoint Subdata(view<queue> queue, view<texture> texture, u32 mip, u32 offset_x,
						  u32 offset_y, u32 offset_z, u32 width, u32 height, u32 depth,
						  const void* data);
} // namespace rt::Texture

#endif /* LEAF_GRAPHICS_TEXTURE_HPP */

// @GPT FIXED: why not namespace rt::Texture ffs
// @GPT FIXED: WHY ARE YOU SPLITTING DECLARATIONS OVER MULTIPLE LINES
