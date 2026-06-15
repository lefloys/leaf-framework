#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <thread>

namespace lf {
	class RateMeter {
	  public:
		using clock = std::chrono::steady_clock;

		void reset() {
			window_start = clock::now();
			next_tick = window_start;
			sample_count = 0;
			current_rate = 0.0;
		}

		void limit(double target_hz) {
			if (target_hz > 0.0) {
				target_interval = std::chrono::duration_cast<clock::duration>(
					std::chrono::duration<double>(1.0 / target_hz));
			} else {
				target_interval.reset();
			}
			next_tick = clock::now();
			primed = false;
		}

		void mark() {
			const clock::time_point now = clock::now();
			++sample_count;
			const std::chrono::duration<double> elapsed = now - window_start;
			if (elapsed.count() >= 1.0) {
				current_rate = static_cast<double>(sample_count) / elapsed.count();
				sample_count = 0;
				window_start = now;
			}
		}

		double rate() const {
			return current_rate;
		}

		void wait() {
			if (!target_interval) {
				return;
			}
			const clock::time_point now = clock::now();
			if (!primed) {
				next_tick = now + *target_interval;
				primed = true;
				return;
			}
			if (now < next_tick) {
				std::this_thread::sleep_until(next_tick);
				return;
			}
			do {
				next_tick += *target_interval;
			} while (next_tick <= now);
		}

	  private:
		clock::time_point window_start = clock::now();
		clock::time_point next_tick = window_start;
		std::optional<clock::duration> target_interval;
		std::size_t sample_count = 0;
		double current_rate = 0.0;
		bool primed = false;
	};
} // namespace lf
