#include "texture_prototype.hpp"

#include <leaf/core/exception.hpp>

#include <algorithm>

bool has_field(const lf::dict& data, lf::string_view field_name) {
	return data.find(field_name) != data.end();
}

lf::TextureSourceFrame parse_frame(const lf::object& object, const lf::string& default_path) {
	lf::TextureSourceFrame frame;
	frame.path = default_path;
	if (object.is<lf::string>()) {
		frame.path = object.as<lf::string>();
		return frame;
	}
	if (!object.is<lf::dict>()) {
		throw lf::runtime_exception(lf::format("texture frame must be a path string or dictionary, got '{}'", object.current_type_name()));
	}

	const lf::dict& data = object.get<lf::dict>();
	if (has_field(data, "path")) {
		frame.path = data.parse_field<lf::string>("path");
	}

	bool has_x = has_field(data, "x");
	bool has_y = has_field(data, "y");
	bool has_width = has_field(data, "w") || has_field(data, "width");
	bool has_height = has_field(data, "h") || has_field(data, "height");
	if (has_x || has_y || has_width || has_height) {
		if (!has_x || !has_y || !has_width || !has_height) {
			throw lf::runtime_exception("texture frame rect needs x, y, and w/h or width/height");
		}
		frame.x = data.parse_field<u32>("x");
		frame.y = data.parse_field<u32>("y");
		frame.width = has_field(data, "w") ? data.parse_field<u32>("w") : data.parse_field<u32>("width");
		frame.height = has_field(data, "h") ? data.parse_field<u32>("h") : data.parse_field<u32>("height");
		frame.has_rect = true;
	}
	return frame;
}

namespace lf {
	TexturePrototype::TexturePrototype(const dict& data) : Prototype(data) {
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
				const list& list = value.get<lf::list>();
				frames.reserve(list.size());
				for (const object& frame : list) {
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
				throw runtime_exception(format("texture frames must be a list or count, got '{}'", value.current_type_name()));
			}
		}
		if (frames.empty()) {
			TextureSourceFrame frame;
			frame.path = path;
			frames.push_back(frame);
		}
	}
} // namespace lf
