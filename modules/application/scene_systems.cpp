#include "scene_systems.hpp"

#include "leaf/core/format.hpp"
#include "leaf/core/filesystem.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/script/virtual_filesystem.hpp"

#include <fstream>
namespace lf {
	SceneDocument::SceneDocument(const Rml::String& tag)
		: Rml::ElementDocument(tag) {}

	void SceneDocument::LoadInlineScript(const Rml::String& content, const Rml::String& source_path, int source_line) {
		scripts.push_back({
			lf::format("{}:{}", source_path, source_line),
			string(content),
		});
	}

	void SceneDocument::LoadExternalScript(const Rml::String& source_path) {
		std::ofstream trace(fs::folder::appdata / "scene-document-trace.log", std::ios::app);
		trace << "external-script " << source_path << '\n';
		trace.flush();
		report<string> source = ReadVirtualTextFile(string(source_path));
		if (!source) {
			load_error = source.error().add_context(lf::format("loading Rml script source '{}'", source_path)).message;
			return;
		}
		log::Info("[scene] loaded script '{}'", source_path);
		log::Logger::instance().flush();
		scripts.push_back({ string(source_path), std::move(*source) });
	}

	report<vector<SceneDocumentScript>> SceneDocument::take_scripts() {
		if (!load_error.empty()) {
			return unexpected(error(generic_errc::input_error, std::move(load_error)));
		}
		return std::move(scripts);
	}

} // namespace lf
