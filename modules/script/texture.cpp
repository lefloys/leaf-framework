#include "leaf/resource/prototypes/texture.hpp"

#include "leaf/core/filesystem.hpp"
#include "leaf/graphics/queue.hpp"
#include "leaf/resource/database.hpp"
#include "leaf/script/virtual_filesystem.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace lf {
	TexturePrototype::TexturePrototype(const dict& data)
		: Prototype<identifier<TexturePrototype, u16, void>>{ data } {
		data.assign(schema(*this));
		if (frames.empty()) {
			frames.push_back({ .path = path });
		}
	}

	TexturePrototype::~TexturePrototype() = default;

	error TexturePrototype::BuildAtlas(rt::view<rt::queue> queue, const std::function<void(size_t, size_t)>& progress, const std::function<void(string_view)>& phase) {
		const auto atlas_start = std::chrono::steady_clock::now();
		vector<atlas_source_frame> source_frames;
		const auto& textures = Database<TexturePrototype>::prototypes;
		for (size_t texture_index = 0; texture_index < textures.size(); ++texture_index) {
			const TexturePrototype& texture = textures[texture_index];
			for (size_t frame_index = 0; frame_index < texture.frames.size(); ++frame_index) {
				const TextureSourceFrame& frame = texture.frames[frame_index];
				if (frame.path.empty()) {
					continue;
				}
				report<fs::path> resolved = ResolveVirtualPathReport(frame.path);
				if (!resolved) {
					return resolved.error().add_context(lf::format("loading texture frame '{}'", frame.path));
				}
				if (!fs::exists(*resolved)) {
					return error(generic_errc::input_error, lf::format("missing texture frame '{}'", frame.path));
				}
				atlas_source_frame source;
				source.texture_index = static_cast<u32>(texture_index);
				source.frame_index = static_cast<u32>(frame_index);
				source.path = frame.path;
				source.rect = frame.rect.value_or(rect<u32>{});
				source_frames.emplace_back(std::move(source));
			}
		}
		const auto source_collection_done = std::chrono::steady_clock::now();
		log::Info("[textures] atlas source collection: {} frames from {} textures in {:.3f}s", source_frames.size(), textures.size(), std::chrono::duration<f64>(source_collection_done - atlas_start).count());

		texture_atlas_options options;
		options.progress = progress;
		options.phase = phase;
		log::Info("[textures] atlas build begin: {} source frames", source_frames.size());
		atlas = build_texture_atlas(queue, source_frames, options);
		const auto atlas_done = std::chrono::steady_clock::now();
		log::Info("[textures] atlas build done: {} packed frames in {:.3f}s", atlas.frames.size(), std::chrono::duration<f64>(atlas_done - source_collection_done).count());
		for (TexturePrototype& texture : Database<TexturePrototype>::prototypes) {
			texture.atlas_frames.clear();
		}
		for (const packed_atlas_frame& frame : atlas.frames) {
			if (frame.texture_index < Database<TexturePrototype>::prototypes.size()) {
				auto& atlas_frames = Database<TexturePrototype>::prototypes[frame.texture_index].atlas_frames;
				if (frame.frame_index >= atlas_frames.size()) {
					atlas_frames.resize(static_cast<size_t>(frame.frame_index) + 1u);
				}
				atlas_frames[frame.frame_index] = frame.rect;
			}
		}
		const auto mapping_done = std::chrono::steady_clock::now();
		log::Info("[textures] atlas frame mapping done in {:.3f}s, total {:.3f}s", std::chrono::duration<f64>(mapping_done - atlas_done).count(), std::chrono::duration<f64>(mapping_done - atlas_start).count());
		return error::no_error;
	}

	void TexturePrototype::ClearAtlas() {
		atlas = {};
		for (TexturePrototype& texture : Database<TexturePrototype>::prototypes) {
			texture.atlas_frames.clear();
		}
	}
} // namespace lf
