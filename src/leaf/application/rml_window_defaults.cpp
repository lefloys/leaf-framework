#include "rml_window_defaults.hpp"

#include "rml_window.hpp"

namespace lf::rml_window_detail {
	string_view default_window_styles() {
		return R"rml(
<style id="rml-window-defaults">
window {
	display: block;
	position: absolute;
	left: 32px;
	top: 32px;
	width: 640px;
	height: 420px;
	min-width: 180px;
	min-height: 120px;
	background: #120A14;
	border-width: 3px;
	border-color: #3B1F2D;
	border-top-color: #C33A4A;
	border-left-color: #3B1F2D;
	border-right-color: #050407;
	border-bottom-color: #050407;
	border-radius: 6px;
	color: #F4EEF2;
	z-index: 1;
}
window.window-focused {
	border-color: #C33A4A;
	border-top-color: #E06A5F;
	border-left-color: #C33A4A;
	border-right-color: #050407;
	border-bottom-color: #050407;
}
window.window-collapsed {
	min-height: 0px;
	max-height: none;
}
window-title {
	display: block;
	height: 40px;
	padding: 10px 86px 0px 16px;
	background: #1D101F;
	border-width: 0px 0px 2px 0px;
	border-bottom-color: #3B1F2D;
	border-radius: 4px;
	color: #F4EEF2;
	font-size: 18px;
	overflow: hidden;
}
window.window-focused window-title {
	background: #1D101F;
	border-bottom-color: #E06A5F;
	color: #F4EEF2;
}
window-title button {
	position: absolute;
	right: 10px;
	top: 7px;
	width: 28px;
	height: 28px;
	padding: 0px;
	margin: 0px;
	background: #0B070D;
	background-color: #0B070D;
	decorator: none;
	border-width: 3px;
	border-color: #3B1F2D;
	border-top-color: #C33A4A;
	border-left-color: #C33A4A;
	border-right-color: #050407;
	border-bottom-color: #050407;
	border-radius: 6px;
	color: #F4EEF2;
	font-size: 0px;
	line-height: 0px;
	text-align: center;
}
window-title button:hover {
	background: #5B1F32;
	background-color: #5B1F32;
	border-top-color: #E06A5F;
	border-left-color: #E06A5F;
	color: #F4EEF2;
}
window-title button:active,
window-title button.window-button-pressed {
	background: #E06A5F;
	background-color: #E06A5F;
	border-top-color: #050407;
	border-left-color: #050407;
	border-right-color: #C33A4A;
	border-bottom-color: #F4EEF2;
	color: #050407;
}
window-title button.window-collapse-button {
	right: 45px;
	padding: 0px;
}
window-icon {
	display: block;
	position: absolute;
	left: 4px;
	top: 4px;
	width: 20px;
	height: 20px;
}
window-pixel {
	display: block;
	position: absolute;
	width: 4px;
	height: 4px;
	background: #F4EEF2;
	background-color: #F4EEF2;
	border-radius: 1px;
}
window-title button:hover window-pixel {
	background: #F4EEF2;
	background-color: #F4EEF2;
}
window-title button:active window-pixel,
window-title button.window-button-pressed window-pixel {
	background: #050407;
	background-color: #050407;
}
window.window-collapsed window-body,
window.window-collapsed window-resize {
	display: none;
}
window-body {
	display: block;
	position: absolute;
	left: 14px;
	right: 14px;
	top: 56px;
	bottom: 14px;
	overflow: auto;
}
window-resize {
	display: block;
	position: absolute;
	background: transparent;
}
window-resize[direction=n] {
	left: 10px;
	right: 10px;
	top: -5px;
	height: 10px;
}
window-resize[direction=e] {
	right: -5px;
	top: 10px;
	bottom: 10px;
	width: 10px;
}
window-resize[direction=s] {
	left: 10px;
	right: 10px;
	bottom: -5px;
	height: 10px;
}
window-resize[direction=w] {
	left: -5px;
	top: 10px;
	bottom: 10px;
	width: 10px;
}
window-resize[direction=ne] {
	right: -5px;
	top: -5px;
	width: 16px;
	height: 16px;
}
window-resize[direction=se] {
	right: -5px;
	bottom: -5px;
	width: 16px;
	height: 16px;
	background: #C33A4A;
}
window-resize[direction=sw] {
	left: -5px;
	bottom: -5px;
	width: 16px;
	height: 16px;
}
window-resize[direction=nw] {
	left: -5px;
	top: -5px;
	width: 16px;
	height: 16px;
}
window[resizable=false] window-resize,
window[resizable=0] window-resize,
window[resizable=disabled] window-resize,
window[resizable=no] window-resize,
window[resizable=off] window-resize {
	display: none;
}
keybind {
	display: none;
}
</style>
)rml";
	}

	string_view close_icon_rml() {
		return R"rml(<window-icon><window-pixel style="left:4px; top:4px;"></window-pixel><window-pixel style="left:8px; top:8px;"></window-pixel><window-pixel style="left:12px; top:12px;"></window-pixel><window-pixel style="left:12px; top:4px;"></window-pixel><window-pixel style="left:4px; top:12px;"></window-pixel></window-icon>)rml";
	}

	string_view collapse_down_icon_rml() {
		return R"rml(
<window-icon>
	<window-pixel style="left:0px; top:4px;"></window-pixel>
	<window-pixel style="left:4px; top:8px;"></window-pixel>
	<window-pixel style="left:8px; top:12px;"></window-pixel>
	<window-pixel style="left:12px; top:8px;"></window-pixel>
	<window-pixel style="left:16px; top:4px;"></window-pixel>
</window-icon>
)rml";
	}

	string_view collapse_up_icon_rml() {
		return R"rml(
<window-icon>
	<window-pixel style="left:0px; top:12px;"></window-pixel>
	<window-pixel style="left:4px; top:8px;"></window-pixel>
	<window-pixel style="left:8px; top:4px;"></window-pixel>
	<window-pixel style="left:12px; top:8px;"></window-pixel>
	<window-pixel style="left:16px; top:12px;"></window-pixel>
</window-icon>
)rml";
	}
}

namespace lf {
	string install_rml_window_defaults(string_view source) {
		constexpr string_view marker = "rml-window-defaults";
		if (source.find(marker) != string_view::npos) {
			return string(source);
		}

		constexpr string_view head_open = "<head>";
		size_t insert_at = source.find(head_open);
		if (insert_at != string_view::npos) {
			insert_at += head_open.size();
		} else {
			insert_at = source.find("</head>");
		}
		if (insert_at == string_view::npos) {
			return string(rml_window_detail::default_window_styles()) + string(source);
		}

		string out;
		out.reserve(source.size() + rml_window_detail::default_window_styles().size());
		out += source.substr(0, insert_at);
		out += rml_window_detail::default_window_styles();
		out += source.substr(insert_at);
		return out;
	}
}
