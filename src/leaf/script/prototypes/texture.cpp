#include "texture.hpp"

#include "leaf/core/exception.hpp"
#include "leaf/core/filesystem.hpp"
#include "leaf/graphics/queue.hpp"
#include "leaf/script/database.hpp"

#include <algorithm>

namespace lf {
	bool has_field(const dict& data, string_view field_name) {
		return data.find(field_name) != data.end();
	}

	TextureSourceFrame parse_frame(const object& object, const string& default_path) {
		TextureSourceFrame frame;
		frame.path = default_path;
		if (object.is<string>()) {
			frame.path = object.as<string>();
			return frame;
		}
		if (!object.is<dict>()) {
			throw runtime_exception(lf::format("texture frame must be a path string or dictionary, got '{}'", object.current_type_name()));
		}

		const dict& data = object.get<dict>();
		if (has_field(data, "path")) {
			frame.path = data.parse_field<string>("path");
		}

		bool has_x = has_field(data, "x");
		bool has_y = has_field(data, "y");
		bool has_width = has_field(data, "w") || has_field(data, "width");
		bool has_height = has_field(data, "h") || has_field(data, "height");
		if (has_x || has_y || has_width || has_height) {
			if (!has_x || !has_y || !has_width || !has_height) {
				throw runtime_exception("texture frame rect needs x, y, and w/h or width/height");
			}
			frame.rect = rect<u32>{
				{ data.parse_field<u32>("x"), data.parse_field<u32>("y") },
				{ has_field(data, "w") ? data.parse_field<u32>("w") : data.parse_field<u32>("width"),
				  has_field(data, "h") ? data.parse_field<u32>("h") : data.parse_field<u32>("height") },
			};
		}
		return frame;
	}

	TexturePrototype::TexturePrototype(const dict& data)
			: Prototype<identifier<TexturePrototype, u16, void>>(data) {
		if (has_field(data, "path")) {
			load_field(data, "path", path);
		}
		if (has_field(data, "world_size")) {
			load_field(data, "world_size", world_size);
		}
		if (has_field(data, "fps")) {
			load_field(data, "fps", frames_per_second);
		}
		if (has_field(data, "frames")) {
			const object& value = data.at("frames");
			if (value.is<list>()) {
				const list& frame_list = value.get<list>();
				frames.reserve(frame_list.size());
				for (const object& frame : frame_list) {
					frames.push_back(parse_frame(frame, path));
				}
			} else if (value.convertible<u16>()) {
				u16 count = std::max<u16>(value.as<u16>(), 1);
				frames.reserve(count);
				for (u16 i = 0; i < count; ++i) {
					TextureSourceFrame frame;
					frame.path = path;
					frames.push_back(frame);
				}
			} else {
				throw runtime_exception(lf::format("texture frames must be a list or count, got '{}'", value.current_type_name()));
			}
		}
		if (frames.empty()) {
			TextureSourceFrame frame;
			frame.path = path;
			frames.push_back(frame);
		}
	}

	error TexturePrototype::BuildAtlas(view<queue> queue) {
		vector<atlas_source_frame> source_frames;
		const auto& textures = Database<TexturePrototype>::prototypes;
		for (size_t texture_index = 0; texture_index < textures.size(); ++texture_index) {
			const TexturePrototype& texture = textures[texture_index];
			for (size_t frame_index = 0; frame_index < texture.frames.size(); ++frame_index) {
				const TextureSourceFrame& frame = texture.frames[frame_index];
				atlas_source_frame source;
				source.texture_index = static_cast<u32>(texture_index);
				source.frame_index = static_cast<u32>(frame_index);
				source.path = frame.path;
				if (frame.rect) {
					source.rect = *frame.rect;
				}
				source_frames.push_back(source);
			}
		}

		atlas = build_texture_atlas(queue, source_frames);
		for (TexturePrototype& texture : Database<TexturePrototype>::prototypes) {
			texture.atlas_frames.clear();
		}
		for (const packed_atlas_frame& frame : atlas.frames) {
			if (frame.texture_index < Database<TexturePrototype>::prototypes.size()) {
				Database<TexturePrototype>::prototypes[frame.texture_index].atlas_frames.push_back(frame.rect);
			}
		}
		return error::no_error;
	}

	void TexturePrototype::ClearAtlas() {
		atlas = {};
		for (TexturePrototype& texture : Database<TexturePrototype>::prototypes) {
			texture.atlas_frames.clear();
		}
	}
} // namespace lf

