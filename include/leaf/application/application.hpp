#pragma once

#include <leaf/core/singleton.hpp>
#include <leaf/core/vector.hpp>

namespace lf {
	using rml_element_installer = void (*)();

	// Process-lifetime application registrations. Installers may be declared at
	// CRT time; Leaf invokes them only after RmlUi itself is initialized.
	struct Application : Singleton<Application> {
		static void install_rml_element(rml_element_installer installer);
		static void install_rml_elements();

		vector<rml_element_installer> rml_element_installers;
	};
}
