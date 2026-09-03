#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/memory.hpp"
#include "leaf/core/rate.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/time.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/application/window.hpp"
#include "leaf/script/state.hpp"

#include <RmlUi/Core/EventListener.h>

namespace Rml {
	class Context;
	class ElementDocument;
	class Event;
}

namespace lf {
	class Scene final : private Rml::EventListener {
	  public:
		explicit Scene(Window& window);
		~Scene();

		void show();
		bool update();

		void set_rml(string_view source);
		Rml::ElementDocument& document();
		sol::state& script_state();
		error execute_document_scripts();

		void set_render_rate(frequency rate);
		frequency render_rate() const;

		Window& window();
		const Window& window() const;

	  private:
		struct ScriptSource {
			string name;
			string text;
		};

		void input();
		void render();
		void unload_document();
		bool execute_script(string_view source, string_view source_name);
		void ProcessEvent(Rml::Event& event) override;

		Window& display;
		Rml::Context* context = nullptr;
		Rml::ElementDocument* rml_document = nullptr;
		sol::state lua = CreateState();
		vector<ScriptSource> document_scripts;
		RateMeter frame_rate;
	};
} // namespace lf
