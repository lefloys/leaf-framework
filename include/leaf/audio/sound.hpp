#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/span.hpp>
#include <leaf/core/types.hpp>
#include <leaf/core/vector.hpp>

namespace lf {
	struct Sound {
		vector<f32> samples;
		u32 channels{};
		u32 sample_rate{};
	};

	report<Sound> LoadSound(span<const byte> bytes);
	error PlaySound(const Sound& sound, f32 volume);
}

