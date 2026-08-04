#include "leaf/application/application.hpp"

namespace lf {
	void Application::install_rml_element(rml_element_installer installer) {
		instance().rml_element_installers.push_back(installer);
	}

	void Application::install_rml_elements() {
		for (rml_element_installer installer : instance().rml_element_installers) {
			installer();
		}
	}
}
