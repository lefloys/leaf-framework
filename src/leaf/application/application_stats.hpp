#pragma once

#include <leaf/core/types.hpp>

#include <atomic>

namespace lf {
	constexpr f32 default_max_fps = 60.0f;

	struct ApplicationStats {
		std::atomic<f32> max_fps = default_max_fps;
		std::atomic<u32> updates_per_second = 60;
		std::atomic<f32> current_fps = 0.0f;
		std::atomic<f32> current_ups = 0.0f;
		std::atomic<bool> render_profile_enabled = false;
	};
}
