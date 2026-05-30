#pragma once

#include <leaf/core/string.hpp>

namespace Rml {
	class ElementDocument;
}

namespace lf {
	void RegisterRmlWindowElement();
	void ReleaseRmlWindowDocumentEvents(Rml::ElementDocument& document);
	string install_rml_window_defaults(string_view source);
}
