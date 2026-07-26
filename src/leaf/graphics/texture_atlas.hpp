#pragma once

#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/graphics/resource.hpp"
#include "leaf/graphics/texture.hpp"
#include "leaf/graphics/texture_view.hpp"
#include "leaf/math/rect.hpp"

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
// @GPT FIXED: what do these mean. texture and frame index...
// @GPT FIXED: why u32 here but f32 below. you should use f32 everywhere
// @GPT FIXED: why arent you using the new progress things. the one where you pass it as progress() which creates a subnode etc
// @GPT FIXED: why are you containing packed atlas frames it should just be an id to a rect ??
// @GPT FIXED: what. this is way too much work in a single function.
