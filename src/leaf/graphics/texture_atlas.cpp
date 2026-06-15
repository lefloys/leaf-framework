#include "texture_atlas.hpp"

#include "leaf/core/exception.hpp"
#include "leaf/core/format.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/script/virtual_filesystem.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_image.h>
#include <stb_rect_pack.h>

namespace lf {
	struct loaded_atlas_frame {
		u32 texture_index = 0;
		u32 frame_index = 0;
		vector<u08> pixels;
		i32 width = 1;
		i32 height = 1;
		stbrp_rect rect{};
	};

	loaded_atlas_frame make_fallback_frame(u32 texture_index, u32 frame_index) {
		loaded_atlas_frame frame;
		frame.texture_index = texture_index;
		frame.frame_index = frame_index;
		frame.width = 1;
		frame.height = 1;
		frame.pixels.push_back(255);
		frame.pixels.push_back(255);
		frame.pixels.push_back(255);
		frame.pixels.push_back(255);
		return frame;
	}

	loaded_atlas_frame load_frame(const atlas_source_frame& source, const texture_atlas_options& options) {
		if (source.path.empty()) {
			return make_fallback_frame(source.texture_index, source.frame_index);
		}

		fs::path path;
		try {
			path = ResolveVirtualPath(source.path);
		} catch (const lf::exception& e) {
			log::Warning("[textures] failed to resolve texture frame '{}': {}", source.path, e.what());
			return make_fallback_frame(source.texture_index, source.frame_index);
		}
		int image_width = 0;
		int image_height = 0;
		int components = 0;
		std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> image(
			stbi_load(path.string().c_str(), &image_width, &image_height, &components, 4),
			stbi_image_free);
		if (!image || image_width <= 0 || image_height <= 0) {
			log::Warning("[textures] missing texture frame '{}', using fallback", path.string());
			return make_fallback_frame(source.texture_index, source.frame_index);
		}

		u32 source_x = source.rect ? source.rect->pos.x : 0;
		u32 source_y = source.rect ? source.rect->pos.y : 0;
		u32 source_width = source.rect ? source.rect->dim.width : static_cast<u32>(image_width);
		u32 source_height = source.rect ? source.rect->dim.height : static_cast<u32>(image_height);
		if (source_x >= static_cast<u32>(image_width) || source_y >= static_cast<u32>(image_height) ||
			source_width == 0 || source_height == 0) {
			log::Warning("[textures] invalid frame rect in '{}', using fallback", path.string());
			return make_fallback_frame(source.texture_index, source.frame_index);
		}

		source_width = std::min(source_width, static_cast<u32>(image_width) - source_x);
		source_height = std::min(source_height, static_cast<u32>(image_height) - source_y);

		loaded_atlas_frame frame;
		frame.texture_index = source.texture_index;
		frame.frame_index = source.frame_index;
		frame.width = static_cast<i32>(source_width);
		frame.height = static_cast<i32>(source_height);
		frame.pixels.resize(static_cast<size_t>(source_width) * static_cast<size_t>(source_height) * 4u);
		for (u32 y = 0; y < source_height; ++y) {
			size_t src_offset = (static_cast<size_t>(source_x) +
								 static_cast<size_t>(source_y + y) * static_cast<size_t>(image_width)) *
								4u;
			size_t dst_offset = static_cast<size_t>(y) * static_cast<size_t>(source_width) * 4u;
			std::memcpy(frame.pixels.data() + dst_offset, image.get() + src_offset,
						static_cast<size_t>(source_width) * 4u);
		}
		return frame;
	}

	u32 next_power_of_two(u32 value) {
		u32 result = 1;
		while (result < value) {
			result *= 2;
		}
		return result;
	}

	void pack_frames(vector<loaded_atlas_frame>& frames, u32& width, u32& height,
					 const texture_atlas_options& options) {
		const i32 padding = static_cast<i32>(options.padding);
		u64 area = 0;
		u32 max_width = 1;
		for (const loaded_atlas_frame& frame : frames) {
			u32 padded_width = static_cast<u32>(frame.width + padding * 2);
			u32 padded_height = static_cast<u32>(frame.height + padding * 2);
			area += static_cast<u64>(padded_width) * static_cast<u64>(padded_height);
			max_width = std::max(max_width, padded_width);
		}
		width = next_power_of_two(
			std::max(options.minimum_extent, static_cast<u32>(std::ceil(std::sqrt(static_cast<f64>(area))))));
		width = std::max(width, next_power_of_two(max_width));
		height = width;

		for (;;) {
			std::vector<stbrp_node> nodes(width);
			stbrp_context context{};
			stbrp_init_target(&context, static_cast<int>(width), static_cast<int>(height), nodes.data(),
							  static_cast<int>(nodes.size()));
			std::vector<stbrp_rect> rects(frames.size());
			for (size_t i = 0; i < frames.size(); ++i) {
				rects[i].id = static_cast<int>(i);
				rects[i].w = static_cast<unsigned short>(frames[i].width + padding * 2);
				rects[i].h = static_cast<unsigned short>(frames[i].height + padding * 2);
			}
			if (stbrp_pack_rects(&context, rects.data(), static_cast<int>(rects.size()))) {
				for (const stbrp_rect& rect : rects) {
					frames[static_cast<size_t>(rect.id)].rect = rect;
				}
				return;
			}
			if (width <= height) {
				width *= 2;
			} else {
				height *= 2;
			}
		}
	}

	void blit_frame(vector<u08>& atlas_pixels, u32 atlas_width, u32 atlas_height, const loaded_atlas_frame& frame,
					u32 padding) {
		const i32 pad = static_cast<i32>(padding);
		i32 padded_width = frame.width + pad * 2;
		i32 padded_height = frame.height + pad * 2;
		if (frame.rect.x < 0 || frame.rect.y < 0 ||
			frame.rect.x + padded_width > static_cast<i32>(atlas_width) ||
			frame.rect.y + padded_height > static_cast<i32>(atlas_height)) {
			log::Warning("[textures] packed frame {}:{} is outside atlas bounds; skipping",
				frame.texture_index, frame.frame_index);
			return;
		}

		for (i32 y = 0; y < padded_height; ++y) {
			i32 src_y = std::clamp(y - pad, 0, frame.height - 1);
			for (i32 x = 0; x < padded_width; ++x) {
				i32 src_x = std::clamp(x - pad, 0, frame.width - 1);
				size_t src_offset =
					(static_cast<size_t>(src_x) + static_cast<size_t>(src_y) * static_cast<size_t>(frame.width)) *
					4u;
				size_t dst_offset = (static_cast<size_t>(frame.rect.x + x) +
									 static_cast<size_t>(frame.rect.y + y) * static_cast<size_t>(atlas_width)) *
									4u;
				std::memcpy(atlas_pixels.data() + dst_offset, frame.pixels.data() + src_offset, 4u);
			}
		}
	}

	packed_atlas_frame packed_frame_from(const loaded_atlas_frame& frame, u32 atlas_width,
										 u32 atlas_height, u32 padding) {
		packed_atlas_frame packed;
		packed.texture_index = frame.texture_index;
		packed.frame_index = frame.frame_index;
		packed.rect = {
			static_cast<f32>(frame.rect.x + static_cast<i32>(padding)) / static_cast<f32>(atlas_width),
			static_cast<f32>(frame.rect.y + static_cast<i32>(padding)) / static_cast<f32>(atlas_height),
			static_cast<f32>(frame.width) / static_cast<f32>(atlas_width),
			static_cast<f32>(frame.height) / static_cast<f32>(atlas_height),
		};
		return packed;
	}

	texture_atlas build_texture_atlas(view<queue> queue, span<const atlas_source_frame> source_frames, texture_atlas_options options) {
		vector<loaded_atlas_frame> loaded_frames;
		loaded_frames.reserve(source_frames.size());
		for (const atlas_source_frame& source : source_frames) {
			loaded_frames.push_back(load_frame(source, options));
		}
		if (loaded_frames.empty()) {
			loaded_frames.push_back(make_fallback_frame(0, 0));
		}

		texture_atlas atlas;
		pack_frames(loaded_frames, atlas.width, atlas.height, options);
		auto atlas_pixels = vector<u08>{};
		atlas_pixels.resize(static_cast<size_t>(atlas.width) * static_cast<size_t>(atlas.height) * 4u, 0);
		for (const loaded_atlas_frame& frame : loaded_frames) {
			blit_frame(atlas_pixels, atlas.width, atlas.height, frame, options.padding);
			atlas.frames.push_back(packed_frame_from(frame, atlas.width, atlas.height, options.padding));
		}

		atlas.atlas_texture = unique(Texture::Create());
		Texture::Data(queue, view<texture>(atlas.atlas_texture), RT_TEXTURE_2D, 0, atlas.width, atlas.height, 1, RT_RGBA8_UNORM,
					  atlas_pixels.data());
		atlas.view.reset(TextureView::CreateFromTexture(atlas.atlas_texture));
		TextureView::Filter(atlas.view, RT_FILTER_NEAREST, RT_FILTER_LINEAR, RT_MIP_FILTER_LINEAR);
		TextureView::Address(atlas.view, RT_ADDRESS_CLAMP, RT_ADDRESS_CLAMP, RT_ADDRESS_CLAMP);
		log::Debug("[textures] atlas {}x{} with {} frames", atlas.width, atlas.height, loaded_frames.size());
		return atlas;
	}
} // namespace lf
