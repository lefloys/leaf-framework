#pragma once

#pragma once

namespace lf {
	struct WindowBackend;
}

namespace lf::glfw {
	const WindowBackend* GetWindowBackend();
}