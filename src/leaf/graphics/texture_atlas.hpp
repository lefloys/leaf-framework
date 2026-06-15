#pragma once

#include "leaf/core/optional.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/graphics/resource.hpp"
#include "leaf/graphics/texture.hpp"
#include "leaf/graphics/texture_view.hpp"
#include "leaf/math/rect.hpp"


namespace lf {
	struct atlas_source_frame {
		u32 texture_index = 0;
		u32 frame_index = 0;
		string path;
		std::optional<rect<u32>> rect;
	};


	struct packed_atlas_frame {
		u32 texture_index = 0;
		u32 frame_index = 0;
		rect<f32> rect{};
	};

	struct texture_atlas_options {
		u32 padding = 2;
		u32 minimum_extent = 64;
	};

	struct texture_atlas {
		unique<texture> atlas_texture;
		unique<texture_view> view;
		vector<packed_atlas_frame> frames;
		u32 width = 1;
		u32 height = 1;
	};

	texture_atlas build_texture_atlas(view<queue> queue, span<const atlas_source_frame> source_frames, texture_atlas_options options = {});
} // namespace lf
