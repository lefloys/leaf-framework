#include "api.hpp"

namespace lf {
	GraphicsAPI Graphics{};

	void SetGraphicsAPI(GraphicsAPI api) {
		Graphics = api;
	}
}

namespace lf::detail {
	error graphics_init() {
		if (!Graphics.init) {
			return error(generic_errc::missing_field, "no graphics backend selected; call lf::SetGraphicsAPI(...) before lf::Init()");
		}

		return Graphics.init();
	}

	void graphics_exit() {
		if (Graphics.exit) {
			Graphics.exit();
		}
	}
}
