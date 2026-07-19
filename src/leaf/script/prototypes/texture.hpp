 #pragma once

#include "leaf/core/vector.hpp"
#include "leaf/core/error.hpp"
#include "leaf/graphics/texture_atlas.hpp"
#include "leaf/script/prototype.hpp"

namespace lf {
	struct TextureSourceFrame {
		string path;
		std::optional<lf::rect<u32>> rect;
	};

	struct TexturePrototype : public Prototype<identifier<TexturePrototype, u16, void>> {
		TexturePrototype(const dict& data);

		string path;
		f32 world_size = 1.0f;
		f32 frames_per_second = 0.0f;
		vector<TextureSourceFrame> frames;
		vector<rect<f32>> atlas_frames;

		inline static texture_atlas atlas;

		static constexpr string_view type() noexcept { return "texture"; }
		static error BuildAtlas(view<queue> queue);
		static void ClearAtlas();
	};
} // namespace lf
