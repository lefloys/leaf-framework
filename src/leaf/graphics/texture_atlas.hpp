#pragma once

#include <leaf/core/filesystem.hpp>
#include <leaf/core/span.hpp>
#include <leaf/core/string.hpp>
#include <leaf/core/types.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/graphics/resource.hpp>
#include <leaf/graphics/texture.hpp>
#include <leaf/graphics/texture_view.hpp>

namespace lf {
	struct atlas_source_rect {
		u32 x = 0;
		u32 y = 0;
		u32 width = 0;
		u32 height = 0;
		bool enabled = false;
	};

	struct atlas_source_frame {
		u32 texture_index = 0;
		u32 frame_index = 0;
		string path;
		atlas_source_rect rect{};
	};

	struct atlas_frame_rect {
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 width = 1.0f;
		f32 height = 1.0f;
	};

	struct packed_atlas_frame {
		u32 texture_index = 0;
		u32 frame_index = 0;
		atlas_frame_rect rect{};
	};

	struct texture_atlas_options {
		u32 padding = 2;
		u32 minimum_extent = 64;
		fs::path root = fs::folder::install / "data";
	};

	struct texture_atlas {
		unique<texture> atlas_texture;
		unique<texture_view> view;
		vector<u08> pixels;
		vector<packed_atlas_frame> frames;
		u32 width = 1;
		u32 height = 1;
	};

	texture_atlas build_texture_atlas(view<queue> queue, span<const atlas_source_frame> source_frames,
									  texture_atlas_options options = {});
} // namespace lf
