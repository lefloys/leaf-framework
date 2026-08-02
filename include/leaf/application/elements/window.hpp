#pragma once

#include <leaf/core/string.hpp>

namespace Rml {
	class ElementDocument;
}

namespace lf {
	void RegisterWindowElement();
	void ReleaseWindowDocumentEvents(Rml::ElementDocument& document);
	string install_window_defaults(string_view source);
} // namespace lf
