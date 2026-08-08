#pragma once

#include "leaf/core/memory.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/vector.hpp"

#include <RmlUi/Core/ElementDocument.h>

namespace lf {
	struct SceneDocumentScript {
		string source_name;
		string source;
	};

class SceneDocument final : public Rml::ElementDocument {
	  public:
		explicit SceneDocument(const Rml::String& tag);
		void LoadInlineScript(const Rml::String& content, const Rml::String& source_path, int source_line) override;
		void LoadExternalScript(const Rml::String& source_path) override;
		report<vector<SceneDocumentScript>> take_scripts();

	  private:
		vector<SceneDocumentScript> scripts;
		string load_error;
};
} // namespace lf
