#pragma once

#include <leaf/core/error.hpp>
#include <vulkan/vulkan.h>

#include <memory>

namespace lf::vk {
	lf::error init(int argc, char* argv[]);
	void exit();

	struct Context {
		inline static Context* context = nullptr;

		VkInstance vk_instance;
		VkDevice vk_device;
	};
}