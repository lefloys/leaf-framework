#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/distance.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/graphics/texture_atlas.hpp"
#include "leaf/resource/prototype.hpp"

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
		lf::distance distance = lf::distance::from_quantum(1);
		f32 frames_per_second = 0.0f;
		vector<TextureSourceFrame> frames;
		vector<rect<f32>> atlas_frames;
	};

	template<>
	struct schema_trait<TextureSourceFrame> {
		static auto get(auto& value) {
			return group(
				field("path", value.path, value.path),
				field("rect", value.rect, value.rect)
			);
		}
	};

	template<>
	struct object_trait<TextureSourceFrame> {
		static TextureSourceFrame parse(const object& value) {
			if (value.is<string>()) {
				return TextureSourceFrame{ .path = value.as<string>() };
			}
			if (!value.is<dict>()) {
				throw runtime_exception(lf::format("texture frame must be a path string or dictionary, got '{}'", value.current_type_name()));
			}
			TextureSourceFrame frame{};
			value.get<dict>().assign(schema(frame));
			return frame;
		}
	};

	template<>
	struct schema_trait<TexturePrototype> {
		static auto get(auto& value) {
			return group(
				schema(PrototypeBase::base(value)),
				field("path", value.path, value.path),
				field("distance", value.distance),
				field("fps", value.frames_per_second),
				field("frames", value.frames, value.frames)
			);
		}
	};
} // namespace lf
