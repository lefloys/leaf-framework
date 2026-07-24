#ifndef LEAF_GRAPHICS_TEXTURE_VIEW_HPP
#define LEAF_GRAPHICS_TEXTURE_VIEW_HPP

#include <leaf/graphics/resource.hpp>

// @GPT : ffs namespaces
namespace rt {
	namespace TextureView {
		// @GPT : ffs newlines
		handle<texture_view> Create();
		handle<texture_view> CreateFromTexture(view<texture> texture);
		void Destroy(handle<texture_view> texture_view);
		void Filter(view<texture_view> texture_view, rt_filter mag_filter, rt_filter min_filter,
					rt_mip_filter mip_filter);
		void Address(view<texture_view> texture_view, rt_address_mode address_u,
					 rt_address_mode address_v, rt_address_mode address_w);
		void Anisotropy(view<texture_view> texture_view, u32 max_anisotropy);
		void Lod(view<texture_view> texture_view, f32 min_lod, f32 max_lod, f32 lod_bias);
		timepoint CopyToBuffer(view<queue> queue, view<texture_view> texture_view,
							   view<buffer> buffer);
		rt_extent_3d Extent(view<texture_view> texture_view);
	} // namespace TextureView
} // namespace rt

#endif /* LEAF_GRAPHICS_TEXTURE_VIEW_HPP */
