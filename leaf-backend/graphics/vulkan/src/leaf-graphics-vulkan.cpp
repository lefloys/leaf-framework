#include "leaf-graphics-vulkan.hpp"
#include "leaf-graphics-vulkan/core.hpp"
#include "leaf-graphics-vulkan/window.hpp"

#include <leaf/graphics/backend.hpp>
#include <leaf/leaf.hpp>

#include <iostream>
#include <memory>
#include <vulkan/vulkan.h>

#define LF_BACKEND_EXPORT extern "C" __declspec(dllexport)

namespace lf::vk {
	namespace {
		const lf::HostAPI* g_host_api = nullptr;
	}

	void SetHostAPI(const lf::HostAPI* api) {
		g_host_api = api;
	}

	const lf::HostAPI* GetHostAPI() {
		return g_host_api;
	}

    GraphicsBackend get_backend() {
		lf::GraphicsBackend backend;
        backend.init = init;
        backend.exit = exit;
		backend.window_backend = WindowVK::get_backend();
        return backend;
    }


    const GraphicsBackend* GetGraphicsBackend() {
        static GraphicsBackend backend = get_backend();
        return &backend;
    }
}

LF_BACKEND_EXPORT const lf::GraphicsBackend* lf_get_graphics_backend() {
	return lf::vk::GetGraphicsBackend();
}

LF_BACKEND_EXPORT void lf_set_host_api(const lf::HostAPI* api) {
	lf::vk::SetHostAPI(api);
}
