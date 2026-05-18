#include "scene.hpp"

#include "rml_backend.hpp"

#include <leaf/core/exception.hpp>
#include <leaf/core/format.hpp>
#include <leaf/core/messages.hpp>
#include <leaf/graphics/command_buffer.hpp>

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/Factory.h>

#include <algorithm>
#include <utility>

namespace lf {
	Scene::Scene(string_view initial) {
	}

	Scene::~Scene() {
	}
} // namespace lf
