#include "texture.hpp"

namespace lf {
	handle<texture> Texture::Create() {
		rt_texture texture = rtTextureCreate();
		detail::check_rutile_error("failed to create texture");
		return { texture };
	}

	void Texture::Destroy(handle<texture> texture) {
		rtTextureDestroy(texture);
	}

	timepoint Texture::Copy(view<queue> queue, view<texture> src_texture, u32 src_mip,
							view<texture> dst_texture, u32 dst_mip) {
		rt_timepoint timepoint = rtTextureCopy(queue, src_texture, src_mip, dst_texture, dst_mip);
		detail::check_rutile_error("failed to copy texture");
		return timepoint;
	}

	timepoint Texture::Data(view<queue> queue, view<texture> texture, rt_texture_type type, u32 mip,
							u32 offset_x, u32 offset_y, u32 offset_z, rt_format format,
							const void* data) {
		rt_timepoint timepoint =
			rtTextureData(queue, texture, type, mip, offset_x, offset_y, offset_z, format, data);
		detail::check_rutile_error("failed to upload texture data");
		return timepoint;
	}

	timepoint Texture::Subcopy(view<queue> queue, view<texture> src_texture, u32 src_mip, u32 src_x,
							   u32 src_y, u32 src_z, view<texture> dst_texture, u32 dst_mip,
							   u32 dst_x, u32 dst_y, u32 dst_z, u32 width, u32 height, u32 depth) {
		rt_timepoint timepoint =
			rtTextureSubcopy(queue, src_texture, src_mip, src_x, src_y, src_z, dst_texture, dst_mip,
							 dst_x, dst_y, dst_z, width, height, depth);
		detail::check_rutile_error("failed to copy texture region");
		return timepoint;
	}

	timepoint Texture::Subdata(view<queue> queue, view<texture> texture, u32 mip, u32 offset_x,
							   u32 offset_y, u32 offset_z, u32 width, u32 height, u32 depth,
							   const void* data) {
		rt_timepoint timepoint = rtTextureSubdata(queue, texture, mip, offset_x, offset_y, offset_z,
												  width, height, depth, data);
		detail::check_rutile_error("failed to upload texture region");
		return timepoint;
	}
} // namespace lf
