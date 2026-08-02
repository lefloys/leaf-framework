#pragma once

#include "leaf/core/string.hpp"

namespace lf {
	inline string EscapeRmlText(string_view value) {
		string escaped;
		escaped.reserve(value.size());
		for (char c : value) {
			switch (c) {
			case '&': escaped += "&amp;"; break;
			case '<': escaped += "&lt;"; break;
			case '>': escaped += "&gt;"; break;
			case '"': escaped += "&quot;"; break;
			case '\'': escaped += "&#39;"; break;
			default: escaped += c; break;
			}
		}
		return escaped;
	}
} // namespace lf
