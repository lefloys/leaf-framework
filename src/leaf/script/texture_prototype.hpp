 #pragma once

#include <leaf/core/vector.hpp>
#include <leaf/core/error.hpp>
#include <leaf/graphics/texture_atlas.hpp>
#include <leaf/script/prototype.hpp>

namespace lf {
	struct TextureSourceFrame {
		string path;
		u32 x = 0;
		u32 y = 0;
		u32 width = 0;
		u32 height = 0;
		bool has_rect = false;
	};

	using TextureAtlasFrame = atlas_frame_rect;

	struct TexturePrototype : public Prototype<identifier<TexturePrototype, u16, void>> {
		TexturePrototype(const dict& data);

		string path;
		f32 world_size = 1.0f;
		f32 frames_per_second = 0.0f;
		vector<TextureSourceFrame> frames;
		vector<TextureAtlasFrame> atlas_frames;

		inline static texture_atlas atlas;

		static constexpr string_view type() noexcept { return "texture"; }
		static error BuildAtlas(view<queue> queue);
		static void ClearAtlas();
	};
} // namespace lf
