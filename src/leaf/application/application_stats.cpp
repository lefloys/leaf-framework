#include "application_stats.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>

namespace lf {
	namespace {
		std::atomic<f32> max_fps_value = default_max_fps;
		std::atomic<f32> current_fps_value = 0.0f;
		std::atomic<f32> current_ups_value = 0.0f;
		std::atomic<bool> render_profile_enabled = false;
	}

	f32 ClampMaxFps(f32 value) {
		if (!std::isfinite(value)) {
			return default_max_fps;
		}
		return std::max(value, 0.0f);
	}

	void SetApplicationMaxFps(f32 value) {
		max_fps_value.store(ClampMaxFps(value), std::memory_order_relaxed);
	}

	f32 ApplicationMaxFps() {
		return ClampMaxFps(max_fps_value.load(std::memory_order_relaxed));
	}

	void RecordApplicationFps(f32 value) {
		current_fps_value.store(std::max(0.0f, value), std::memory_order_relaxed);
	}

	void RecordApplicationUps(f32 value) {
		current_ups_value.store(std::max(0.0f, value), std::memory_order_relaxed);
	}

	f32 CurrentApplicationFps() {
		return current_fps_value.load(std::memory_order_relaxed);
	}

	f32 CurrentApplicationUps() {
		return current_ups_value.load(std::memory_order_relaxed);
	}

	void SetRenderProfileEnabled(bool enabled) {
		render_profile_enabled.store(enabled, std::memory_order_relaxed);
	}

	bool RenderProfileEnabled() {
		return render_profile_enabled.load(std::memory_order_relaxed);
	}
}
