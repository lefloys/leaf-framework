#pragma once

#include <leaf/core/string.hpp>
#include <leaf/core/types.hpp>

namespace lf::rml_window_detail {
	enum class ResizeEdge : u08 {
		None = 0,
		North = 1 << 0,
		East = 1 << 1,
		South = 1 << 2,
		West = 1 << 3,
	};

	constexpr ResizeEdge operator|(ResizeEdge left, ResizeEdge right) {
		return static_cast<ResizeEdge>(static_cast<u08>(left) | static_cast<u08>(right));
	}

	constexpr ResizeEdge& operator|=(ResizeEdge& left, ResizeEdge right) {
		left = left | right;
		return left;
	}

	constexpr bool has_resize_edge(ResizeEdge edges, ResizeEdge edge) {
		return (static_cast<u08>(edges) & static_cast<u08>(edge)) != 0;
	}

	constexpr bool false_attribute_value(string_view value) {
		return value == "false" ||
			value == "0" ||
			value == "disabled" ||
			value == "no" ||
			value == "off";
	}

	constexpr ResizeEdge resize_edge_from_string(string_view direction) {
		ResizeEdge result = ResizeEdge::None;
		for (char c : direction) {
			switch (c) {
			case 'n': result |= ResizeEdge::North; break;
			case 'e': result |= ResizeEdge::East; break;
			case 's': result |= ResizeEdge::South; break;
			case 'w': result |= ResizeEdge::West; break;
			default: break;
			}
		}
		return result;
	}
}
