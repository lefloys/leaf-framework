#pragma once

namespace lf {
	struct GraphicsBackend;
	struct HostAPI;
}
namespace lf::vk {
	const lf::GraphicsBackend* GetGraphicsBackend();
	void SetHostAPI(const lf::HostAPI* api);
	const lf::HostAPI* GetHostAPI();
}

