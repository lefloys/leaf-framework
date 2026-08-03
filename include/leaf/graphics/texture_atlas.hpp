#pragma once

#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/graphics/resource.hpp"
#include "leaf/graphics/texture.hpp"
#include "leaf/graphics/texture_view.hpp"
#include "leaf/script/math/rect.hpp"

#include <functional>

namespace lf {
	struct atlas_source_frame {
		u32 texture_index = 0;
		u32 frame_index = 0;
		string path;
		lf::rect<u32> rect{};
	};

	struct packed_atlas_frame {
		u32 texture_index = 0;
		u32 frame_index = 0;
		lf::rect<f32> rect{};
	};

	struct texture_atlas_options {
		u32 padding = 2;
		u32 max_frame_extent = 256;
		std::function<void(size_t completed, size_t total)> progress;
		std::function<void(string_view phase)> phase;
	};

	struct texture_atlas {
		rt::unique<rt::texture> atlas_texture;
		rt::unique<rt::texture_view> view;
		vector<packed_atlas_frame> frames;
	};
	texture_atlas build_texture_atlas(rt::view<rt::queue> queue, span<const atlas_source_frame> source_frames, const texture_atlas_options& options = {});
} // namespace lf
