#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/graphics/texture_atlas.hpp"
#include "leaf/script/prototype.hpp"

#include <functional>

namespace lf {
	struct TextureSourceFrame {
		string path;
		std::optional<lf::rect<u32>> rect;
	};

	struct TexturePrototype final : public Prototype<identifier<TexturePrototype, u16, void>> {
		static constexpr string_view type() noexcept { return "texture"; }
		inline static texture_atlas atlas;
		static error BuildAtlas(rt::view<rt::queue> queue, const std::function<void(size_t, size_t)>& progress = {}, const std::function<void(string_view)>& phase = {});
		static void ClearAtlas();

		TexturePrototype(const dict& data);
		~TexturePrototype();

		string path;
		f32 world_size = 1.0f;
		f32 frames_per_second = 0.0f;
		vector<TextureSourceFrame> frames;
		vector<rect<f32>> atlas_frames;
	};
} // namespace lf
