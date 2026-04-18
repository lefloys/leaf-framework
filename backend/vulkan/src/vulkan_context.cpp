#include "vulkan_context.hpp"

#include "resources/window_vk.hpp"

#include <cstdlib>

namespace lf::detail::vk {
	namespace {
		std::unique_ptr<context> context_ptr;
	}

	void context::shutdown() {
		windows.clear_leaked_resources();

		if (instance) {
			vkDestroyInstance(instance, nullptr);
			instance = VK_NULL_HANDLE;
		}
	}

	bool has_context() {
		return static_cast<bool>(context_ptr);
	}

	void create_context() {
		if (!context_ptr) {
			context_ptr = std::make_unique<context>();
		}
	}

	void destroy_context() {
		if (context_ptr) {
			context_ptr->shutdown();
		}

		context_ptr.reset();
	}

	context& get_context() {
		if (!context_ptr) {
			std::abort();
		}

		return *context_ptr;
	}
}
