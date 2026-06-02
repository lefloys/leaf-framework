#pragma once

#include <leaf/core/types.hpp>

#include <atomic>

namespace lf {
	/*!
	** @ingroup application
	** @brief Default render frame-rate cap.
	*/
	constexpr f32 default_max_fps = 60.0f;

	/*!
	** @ingroup application
	** @brief Shared runtime counters and tuning flags for an application.
	**
	** The render and update threads both touch these values, so the fields are
	** atomic and can be sampled safely from the main application API.
	*/
	struct ApplicationStats {
		/*!
		** @brief Target maximum frames per second.
		*/
		std::atomic<f32> max_fps = default_max_fps;

		/*!
		** @brief Target fixed updates per second.
		*/
		std::atomic<u32> updates_per_second = 60;

		/*!
		** @brief Multiplier applied to fixed simulation updates.
		*/
		std::atomic<f32> speed_multiplier = 1.0f;

		/*!
		** @brief Most recently measured frames per second.
		*/
		std::atomic<f32> current_fps = 0.0f;

		/*!
		** @brief Most recently measured fixed updates per second.
		*/
		std::atomic<f32> current_ups = 0.0f;

		/*!
		** @brief Whether render profiling is currently enabled.
		*/
		std::atomic<bool> render_profile_enabled = false;
	};
} // namespace lf
