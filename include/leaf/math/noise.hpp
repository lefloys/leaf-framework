#pragma once

#include <leaf/core/types.hpp>

#include <algorithm>
#include <cmath>
#include <random>

namespace lf {
	inline f32 periodic_value_noise(i32 position, i32 extent, i32 wavelength, u64 seed) {
		if (extent <= 0 || wavelength <= 0) {
			return 0.0f;
		}
		const i32 sample_count = std::max(1, extent / wavelength);
		const i32 wrapped_position = (position % extent + extent) % extent;
		const f32 sample_position = static_cast<f32>(wrapped_position) / static_cast<f32>(extent) * static_cast<f32>(sample_count);
		const i32 sample = static_cast<i32>(std::floor(sample_position));
		const auto value_at = [seed, sample_count](i32 index) {
			const i32 wrapped_index = (index % sample_count + sample_count) % sample_count;
			std::mt19937 random{ static_cast<u32>(wrapped_index) ^ static_cast<u32>(seed) ^ static_cast<u32>(seed >> 32) };
			return static_cast<f32>(random() & 0xffffu) / 32767.5f - 1.0f;
		};
		const f32 left = value_at(sample);
		const f32 right = value_at(sample + 1);
		const f32 fraction = sample_position - static_cast<f32>(sample);
		const f32 blend = fraction * fraction * (3.0f - 2.0f * fraction);
		return left + (right - left) * blend;
	}
}
