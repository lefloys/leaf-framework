#include "leaf/graphics/texture_view.hpp"

#include "leaf/graphics/queue.hpp"

namespace rt {
	handle<texture_view> TextureView::Create() {
		rt_texture_view texture_view = rtTextureViewCreate();
		detail::check_rutile_error("failed to create texture view");
		return { texture_view };
	}

	handle<texture_view> TextureView::CreateFromTexture(view<texture> texture) {
		auto view_handle = Create();
		rtTextureViewBind(view_handle, texture);
		detail::check_rutile_error("failed to bind texture view");
		return view_handle;
	}

	void TextureView::Destroy(handle<texture_view> texture_view) {
		rtTextureViewDestroy(texture_view);
	}
	void TextureView::Filter(view<texture_view> texture_view, rt_filter mag_filter, rt_filter min_filter, rt_mip_filter mip_filter) {
		rtTextureViewFilter(texture_view, mag_filter, min_filter, mip_filter);
		detail::check_rutile_error("failed to set texture view filter");
	}

	void TextureView::Address(view<texture_view> texture_view, rt_address_mode address_u, rt_address_mode address_v, rt_address_mode address_w) {
		rtTextureViewAddress(texture_view, address_u, address_v, address_w);
		detail::check_rutile_error("failed to set texture view address mode");
	}

	void TextureView::Anisotropy(view<texture_view> texture_view, u32 max_anisotropy) {
		rtTextureViewAnisotropy(texture_view, max_anisotropy);
		detail::check_rutile_error("failed to set texture view anisotropy");
	}

	void TextureView::Lod(view<texture_view> texture_view, f32 min_lod, f32 max_lod, f32 lod_bias) {
		rtTextureViewLod(texture_view, min_lod, max_lod, lod_bias);
		detail::check_rutile_error("failed to set texture view LOD");
	}

	timepoint TextureView::CopyToBuffer(view<queue> queue, view<texture_view> texture_view, view<buffer> buffer) {
		auto lock = detail::lock_queue(queue);
		rt_timepoint timepoint = rtTextureViewCopyToBuffer(texture_view, buffer);
		detail::check_rutile_error("failed to copy texture view to buffer");
		return timepoint;
	}

	rt_extent_3d TextureView::Extent(view<texture_view> texture_view) {
		rt_extent_3d extent = rtTextureViewExtent(texture_view);
		detail::check_rutile_error("failed to query texture view extent");
		return extent;
	}
} // namespace rt
