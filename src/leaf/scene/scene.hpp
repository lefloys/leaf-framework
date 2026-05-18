#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/filesystem.hpp>
#include <leaf/core/string.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/graphics/command_buffer.hpp>
#include <leaf/graphics/framebuffer.hpp>
#include <leaf/graphics/resource.hpp>
#include <leaf/graphics/window.hpp>
#include <leaf/math/dim.hpp>

#include <RmlUi/Core/EventListener.h>
#include <sol/sol.hpp>

#include <functional>
#include <memory>

namespace Rml {
	class Context;
	class ElementDocument;
	class ElementInstancer;
}

namespace lf {
	class RmlRenderInterface;

	class Scene {
	public:
		Scene(string_view initial);
		~Scene();


	private:


		sol::state lua;
		Rml::Context* context = nullptr;
		lf::unique<lf::window> display;
	};
} // namespace lf
