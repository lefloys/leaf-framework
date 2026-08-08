#pragma once

#include "leaf/core/time.hpp"
#include "leaf/core/optional.hpp"

#include <chrono>
#include <cstddef>
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
			exact_hertz.reset();
			if (target_hz > 0.0) {
				target_interval = std::chrono::duration_cast<clock::duration>(
					std::chrono::duration<double>(1.0 / target_hz)
				);
			} else {
				target_interval.reset();
			}
			next_tick = clock::now();
			primed = false;
		}

		void limit(frequency target) {
			const i64 raw_hertz = target.hertz().raw();
			if (raw_hertz <= 0 || raw_hertz % fixed::scale != 0) {
				limit(static_cast<double>(raw_hertz) / static_cast<double>(fixed::scale));
				return;
			}
			target_interval.reset();
			exact_hertz = raw_hertz / fixed::scale;
			cadence_start = clock::now();
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
			if (exact_hertz) {
				const clock::time_point now = clock::now();
				if (!primed) {
					cadence_start = now;
					exact_tick = 1;
					primed = true;
					return;
				}
				auto deadline_for = [this](u64 tick) {
					const u64 numerator = tick * static_cast<u64>(clock::period::den);
					const u64 denominator = static_cast<u64>(*exact_hertz) * static_cast<u64>(clock::period::num);
					return cadence_start + clock::duration(static_cast<clock::duration::rep>(numerator / denominator));
				};
				clock::time_point deadline = deadline_for(exact_tick);
				if (now >= deadline) {
					const u64 elapsed = static_cast<u64>((now - cadence_start).count());
					const u64 elapsed_ticks = elapsed * static_cast<u64>(*exact_hertz) * static_cast<u64>(clock::period::num) / static_cast<u64>(clock::period::den);
					exact_tick = elapsed_ticks + 1;
					deadline = deadline_for(exact_tick);
				}
				std::this_thread::sleep_until(deadline);
				++exact_tick;
				return;
			}
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
		clock::time_point cadence_start = window_start;
		clock::time_point next_tick = window_start;
		optional<clock::duration> target_interval;
		optional<i64> exact_hertz;
		u64 exact_tick = 0;
		std::size_t sample_count = 0;
		double current_rate = 0.0;
		bool primed = false;
	};
} // namespace lf
